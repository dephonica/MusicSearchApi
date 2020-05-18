/****
  This file is part of Audio Search Service

  Copyright 2020 Max Klimenko <info@dephonica.com> for dePhonica sound labs.

  NOTICE:  All information contained herein is, and remains the property of dePhonica sound labs
           and its contragents mentioned in related agreements, if any.
****/

#ifndef SESSIONMODEL_H
#define SESSIONMODEL_H

#include <stdio.h>

#include <queue>
#include <unordered_set>

#include <QDate>
#include <QJsonArray>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QString>
#include <QThread>
#include <QWaitCondition>

#include "Configuration.h"
#include "CoreException.h"
#include "CoreInstance.h"
#include "Engine/Fingerprinter.h"
#include "Engine/SearchHashesWorker.h"

#define THREAD_TICK_MILLISECONDS 50
#define MAX_TRACKS_IN_RESULT 20
#define SESSION_TIMEOUT_SECONDS 30
#define SEARCH_WORKERS_COUNT 80

namespace dePhonica::Core::Api {

using namespace dePhonica::Core;
using namespace dePhonica::Buffers;
using namespace dePhonica::MusicSearch;

class SessionModel : public QThread
{
    Q_OBJECT

private:
    QMutex lock_, conditionLock_;

    const ICoreInstance& coreInstance_;
    const QJsonObject sessionInfo_;

    SingleBuffer<PCMTYPE> collectBuffer_;
    QWaitCondition isCollectBufferUpdated_;
    std::queue<uint32_t> requestLengthQueue_;

    enum class SampleTypes
    {
        none,
        f32le,
        s16le
    };

    SampleTypes sampleType_;

    std::vector<LutResult> searchResult_;
    size_t resultVersionIndex_;
    float maxResultDelta_ = 0, sqAverageDelta_ = 0;

    QString sessionLog_;

    MusicSettings musicSettings_;

public:
    SessionModel(const ICoreInstance& coreInstance, const QJsonObject& sessionInfo)
        : lock_(QMutex::Recursive)
        , coreInstance_(coreInstance)
        , sessionInfo_(sessionInfo)
        , collectBuffer_("SessionModel collection buffer", 16000 * 60)
        , sampleType_(SampleTypes::none)
        , resultVersionIndex_(0)
    {
        if (sessionInfo.contains("sampleType") == false)
        {
            throw CoreException("Undefined 'sampleType' property in the session definition. Valid values are: 'f32le', 's16le'");
        }

        auto sampleType = sessionInfo["sampleType"].toString();
        if (sampleType == "f32le")
        {
            sampleType_ = SampleTypes::f32le;
        }
        else if (sampleType == "s16le")
        {
            sampleType_ = SampleTypes::s16le;
        }

        start();
    }

    ~SessionModel()
    {
        coreInstance_.DumpSessionData(collectBuffer_, sessionLog_, sessionInfo_.contains("storeSessionData"));

        requestInterruption();
        wait();
    }

    QJsonObject GetInformation()
    {
        lock_.lock();
        std::vector<LutResult> searchResult(searchResult_);
        size_t resultVersionIndex = resultVersionIndex_;
        lock_.unlock();

        QJsonArray tracksObject;
        for (auto& result : searchResult)
        {
            tracksObject.append(QJsonObject(
                { { "fileIndex", static_cast<int>(result.TrackIndex) },
                  { "fileName", coreInstance_.GetFileNameByIndex(result.TrackIndex) },
                  { "filePositionSeconds",
                    static_cast<double>(result.ChunkIndex * (musicSettings_.SliceDurationSeconds - musicSettings_.SliceOverlapSeconds)) },
                  { "similarity", static_cast<int>(result.Catches) } }));
        }

        return { { "resultVersion", static_cast<int>(resultVersionIndex) },
                 { "resultTracks", tracksObject },
                 { "maxResultDelta", maxResultDelta_ },
                 { "squareAverageDelta", sqAverageDelta_ },
                 { "result", "ok" } };
    }

    QJsonObject PushSamples(const QByteArray& samples)
    {
        std::vector<PCMTYPE> samplesVector;

        if (sampleType_ == SampleTypes::f32le)
        {
            auto samplesCount = samples.size() / sizeof(float);
            auto samplesFloat = reinterpret_cast<const float*>(samples.data());
            std::copy(samplesFloat, samplesFloat + samplesCount, std::back_inserter(samplesVector));
        }
        else if (sampleType_ == SampleTypes::s16le)
        {
            auto samplesCount = samples.size() / sizeof(short);
            auto samplesShort = reinterpret_cast<const short*>(samples.data());

            for (size_t n = 0; n < samplesCount; n++)
            {
                samplesVector.push_back(static_cast<float>(samplesShort[n]) / 32768.0f);
            }
        }
        else
        {
            throw CoreException("Unable to push samples into the session with an invalid session info");
        }

        collectBuffer_.Ensure(collectBuffer_.DataLengthSamples() + samplesVector.size());
        std::copy(samplesVector.begin(), samplesVector.end(), collectBuffer_.BufferData().begin() + collectBuffer_.DataLengthSamples());
        collectBuffer_.DataLengthSamples(collectBuffer_.DataLengthSamples() + samplesVector.size());

        lock_.lock();
        requestLengthQueue_.push(collectBuffer_.DataLengthSamples());
        lock_.unlock();

        conditionLock_.lock();
        isCollectBufferUpdated_.wakeAll();
        conditionLock_.unlock();

        return { { "samplesPushed", static_cast<int>(samplesVector.size()) },
                 { "samplesCollected", static_cast<int>(collectBuffer_.DataLengthSamples()) },
                 { "result", "ok" } };
    }

protected:
    void run()
    {
        qInfo() << "Thread started with ID: " << QThread::currentThreadId();

        Fingerprinter fingerprinter(musicSettings_);

        int timeoutCounter = 0;
        size_t maxTrackCount = coreInstance_.GetMaxTrackCount();
        auto tracksCompareTo = std::make_unique<uint8_t[]>(maxTrackCount);

        auto workers = SearchHashesWorker::AllocateWorkers(SEARCH_WORKERS_COUNT, coreInstance_);

        while (QThread::currentThread()->isInterruptionRequested() == false)
        {
            conditionLock_.lock();
            bool isBufferUpdated = isCollectBufferUpdated_.wait(&conditionLock_, THREAD_TICK_MILLISECONDS);
            conditionLock_.unlock();

            if (isBufferUpdated == false)
            {
                timeoutCounter += THREAD_TICK_MILLISECONDS;
                if ((timeoutCounter / 1000) > SESSION_TIMEOUT_SECONDS)
                {
                    break;
                }

                continue;
            }

            timeoutCounter = 0;

            while (true)
            {
                uint32_t requestLength = 0;

                {
                    QMutexLocker locker(&lock_);

                    if (requestLengthQueue_.size() == 0)
                    {
                        break;
                    }

                    requestLength = requestLengthQueue_.front();
                    requestLengthQueue_.pop();
                }

                for (size_t n = 0; n < maxTrackCount; n++)
                {
                    tracksCompareTo[n] = 1;
                }

                try
                {
                    SingleBuffer<float> croppedBuffer("Session model crop buffer");
                    croppedBuffer.Copy(collectBuffer_.BufferData().data(), requestLength, musicSettings_.TargetSampleRate);
                    croppedBuffer.DataLengthSamples(requestLength);

                    Log(QString("1. Generating fingerprint for fragment %1 ms")
                            .arg(requestLength * 1000 / musicSettings_.TargetSampleRate));

                    auto fragmentPeaks = GenerateFingerprint(fingerprinter, croppedBuffer);

                    std::vector<LutResult> searchResult;
                    searchResult.reserve(maxTrackCount);

                    float maxDelta = 1.0f;
                    float sqAverageDelta = 0;

                    auto tracksCompareToCount = maxTrackCount;

                    searchResult.clear();

                    Log(QString("3.2. Tracks to compare to: %1, peaks to compare: %2").arg(tracksCompareToCount).arg(fragmentPeaks.size()));

                    auto fragmentPeaksGrouped = PeakCompareWorker::GroupPeaks(fragmentPeaks, 1);

                    SearchHashesWorker::ComparePeaks(workers, fragmentPeaksGrouped, tracksCompareTo, maxTrackCount);
                    SearchHashesWorker::WaitAll(workers);

                    SearchHashesWorker::AggregateResultTracks(workers, searchResult, false);

                    Log("4. Calculate approximation.");
                    maxDelta = EstimateApprox(searchResult, sqAverageDelta);

                    Log(QString("5. Max delta: %1").arg(maxDelta));
                    Log("6. Assigning result...");

                    lock_.lock();

                    searchResult_.clear();
                    for (size_t n = 0; n < searchResult.size() && n < MAX_TRACKS_IN_RESULT; n++)
                    {
                        searchResult_.push_back(searchResult[n]);
                    }

                    maxResultDelta_ = maxDelta;
                    sqAverageDelta_ = sqAverageDelta;
                    resultVersionIndex_++;
                    lock_.unlock();

                    Log("7. Done...");
                }
                catch (MusicException* ex)
                {
                    qInfo() << QString("Exception in search thread: ") + ex->what();
                }

                if (searchResult_.size() > 0)
                {
                    Log("8. Top track: " + coreInstance_.GetFileNameByIndex(searchResult_[0].TrackIndex));
                }
            }
        }

        qInfo() << "Thread finished with ID: " << QThread::currentThreadId();
    }

private:
    QDateTime lastLogTimestamp_;

    void Log(QString message)
    {
        auto now = QDateTime::currentDateTime();

        if (lastLogTimestamp_.isNull())
        {
            lastLogTimestamp_ = now;
        }

        auto diffMilliseconds = lastLogTimestamp_.msecsTo(now);

        lastLogTimestamp_ = now;

        auto console = qDebug();
        console.nospace();
        console.noquote();

        auto logLine = QString("[%1 +%2 msec] (%3) - %4")
                           .arg(now.toString())
                           .arg(diffMilliseconds)
                           .arg((size_t)QThread::currentThreadId())
                           .arg(message);

        console << logLine;
        sessionLog_ += logLine + "\n";
    }

    const std::vector<PeakDescription> GenerateFingerprint(Fingerprinter& fingerprinter, SingleBuffer<PCMTYPE>& inputBuffer)
    {
        std::vector<uint8_t> peaksMapped[musicSettings_.FrequencyPoints];

        size_t chunksCount = static_cast<size_t>(inputBuffer.DataLengthSeconds() / static_cast<double>(musicSettings_.SliceDurationSeconds -
                                                                                                       musicSettings_.SliceOverlapSeconds));
        for (size_t n = 0; n < musicSettings_.FrequencyPoints; n++)
        {
            peaksMapped[n].resize(chunksCount);
        }

        SingleBuffer<PCMTYPE> processingBuffer("Processing buffer", inputBuffer.DataLengthSamples() + 32);

        size_t stepsCounter = 0;
        size_t initialOffset = inputBuffer.DataLengthSeconds() > 5 ? inputBuffer.SampleRate() : 0;

        for (size_t offset = 0; offset < 20000; offset += 757, stepsCounter++)
        {
            processingBuffer.Copy(inputBuffer.BufferData().data() + offset + initialOffset,
                                  inputBuffer.DataLengthSamples() - offset - initialOffset,
                                  inputBuffer.SampleRate());

            fingerprinter.Generate(processingBuffer);

            auto fragmentPeaks = fingerprinter.PeaksCollection();

            size_t chunkOffset = static_cast<double>(offset) / inputBuffer.SampleRate() /
                                 static_cast<double>(musicSettings_.SliceDurationSeconds - musicSettings_.SliceOverlapSeconds);

            for (const auto& peak : fragmentPeaks)
            {
                peaksMapped[peak.BandIndex][peak.ChunkIndex + chunkOffset]++;
            }
        }

        Log("2. Collecting fingerprint hashes.");

        std::vector<PeakDescription> resultPeaks;

        for (size_t n = 0; n < musicSettings_.FrequencyPoints; n++)
        {
            const auto& bandVector = peaksMapped[n];

            for (size_t m = 0; m < chunksCount; m++)
            {
                if (bandVector[m] > stepsCounter / 2)
                {
                    PeakDescription resultPeak;
                    resultPeak.BandIndex = n;
                    resultPeak.ChunkIndex = m;
                    resultPeak.PeakCutoffDb = musicSettings_.PeakCutoffThresholdDb;

                    resultPeaks.push_back(resultPeak);
                }
            }
        }

        return resultPeaks;
    }

    void Approximate(const std::vector<LutResult>& y, float& a, float& b)
    {
        double sumX = 0;
        double sumY = 0;
        double sumX2 = 0;
        double sumXY = 0;

        size_t n = y.size();

        for (size_t i = 0; i < n; i++)
        {
            sumX += i;
            sumY += y[i].Catches;
            sumX2 += i * i;
            sumXY += i * y[i].Catches;
        }

        a = (n * sumXY - (sumX * sumY)) / (n * sumX2 - sumX * sumX);
        b = (sumY - a * sumX) / n;
    }

    float EstimateApprox(const std::vector<LutResult>& y, float& sqAverageDelta)
    {
        if (y.size() == 0)
        {
            sqAverageDelta = 0;
            return 0;
        }

        float a, b;
        Approximate(y, a, b);

        double delta = 0;
        double maxDelta = 0;

        for (size_t i = 0; i < y.size(); i++)
        {
            double yc = a * i + b;
            double diff = y[i].Catches / yc;
            delta += diff * diff;

            if (diff > maxDelta)
            {
                maxDelta = diff;
            }
        }

        sqAverageDelta = std::sqrt(delta / y.size());

        return maxDelta;
    }
};

} // namespace dePhonica::Core::Api

#endif // SESSIONMODEL_H
