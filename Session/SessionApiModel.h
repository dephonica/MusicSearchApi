/****
  This file is part of Audio Search Service

  Copyright 2020 Max Klimenko <info@dephonica.com> for dePhonica sound labs.

  NOTICE:  All information contained herein is, and remains the property of dePhonica sound labs
           and its contragents mentioned in related agreements, if any.
****/

#ifndef SESSIONAPIMODEL_H
#define SESSIONAPIMODEL_H

#include <map>
#include <memory>

#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QUuid>

#include "API/Session/SessionModel.h"
#include "CoreException.h"
#include "Interfaces/ICoreInstance.h"

namespace dePhonica::Core::Api {

using namespace dePhonica::Core;
using namespace dePhonica::Core::Interfaces;

class SessionApiModel
{
private:
    QMutex lock_;

    ICoreInstance& coreInstance_;

    std::map<QString, std::unique_ptr<SessionModel>> sessions_;

public:
    SessionApiModel(ICoreInstance& coreInstance)
        : lock_(QMutex::Recursive)
        , coreInstance_(coreInstance)
    {
    }

    QJsonObject CreateSession(const QJsonObject& sessionInfo)
    {
        QMutexLocker locker(&lock_);

        auto token = QUuid::createUuid().toString();
        sessions_[token] = std::make_unique<SessionModel>(coreInstance_, sessionInfo);

        return { { "token", token }, { "result", "ok" } };
    }

    QJsonObject DeleteSession(const QString sessionToken)
    {
        QMutexLocker locker(&lock_);

        auto sessionIterator = sessions_.find(sessionToken);

        if (sessionIterator != sessions_.end())
        {
            sessions_.erase(sessionIterator);
            return { { "result", "ok" } };
        }

        throw CoreException("Unable to find session to remove - token was not found: " + sessionToken);
    }

    QJsonObject GetSessionInfo(const QString sessionToken)
    {
        QMutexLocker locker(&lock_);

        auto sessionIterator = sessions_.find(sessionToken);

        if (sessionIterator != sessions_.end())
        {
            return sessionIterator->second->GetInformation();
        }

        throw CoreException(QString("Unable to retrieve session information - token was not found: " + sessionToken));
    }

    QJsonObject AppendSessionSamples(const QString sessionToken, const QByteArray& samples)
    {
        QMutexLocker locker(&lock_);

        auto sessionIterator = sessions_.find(sessionToken);

        if (sessionIterator != sessions_.end())
        {
            return sessionIterator->second->PushSamples(samples);
        }

        throw CoreException("Unable to push samples to the session - token was not found: " + sessionToken);
    }
};

} // namespace dePhonica::Core::Api

#endif // SESSIONAPIMODEL_H
