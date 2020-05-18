/****
  This file is part of Audio Search Service

  Copyright 2020 Max Klimenko <info@dephonica.com> for dePhonica sound labs.

  NOTICE:  All information contained herein is, and remains the property of dePhonica sound labs
           and its contragents mentioned in related agreements, if any.
****/

#ifndef APIENGINE_H
#define APIENGINE_H

#include <memory>
#include <vector>

#include <QHttpServer>
#include <QJsonArray>
#include <QObject>
#include <QString>

#include "IBaseApiView.h"
#include "CoreException.h"

namespace dePhonica::Core::Api {

class ApiEngine
{
private:
    QHttpServer httpServer_;
    QString basePath_;

    quint16 listenPort_;

public:
    ApiEngine(const QString& basePath, quint16 listenPort);
    ~ApiEngine();

    void AddEndpoint(IBaseApiView& viewInstance, const QString& overrideUriPath = "");

    void Listen();

    static QJsonObject ToError(QString message) { return { { "result", "error" }, { "message", message } }; }

    template<typename T>
    static inline QJsonArray ToJsonArray(const std::vector<T>& elems)
    {
        QJsonArray a;
        std::transform(elems.begin(), elems.end(), std::back_inserter(a), [](const auto& v) { return static_cast<QJsonValue>(v); });
        return a;
    }

    template<typename T>
    static inline void FromJsonArray(const QJsonArray& sourceArray, std::vector<T>& targetVector)
    {
        targetVector.reserve(sourceArray.size());

        if (std::is_same<T, float>::value)
        {
            std::transform(sourceArray.begin(), sourceArray.end(), std::back_inserter(targetVector), [](const QJsonValue& v) {
                return static_cast<float>(v.toDouble());
            });
        }

        if (std::is_same<T, double>::value)
        {
            std::transform(sourceArray.begin(), sourceArray.end(), std::back_inserter(targetVector), [](const QJsonValue& v) {
                return v.toDouble();
            });
        }
    }

    static qint64 ArgumentToId(const QString& argument)
    {
        if (argument == "*")
        {
            return -1;
        }

        bool ok;
        qint64 id = argument.toLongLong(&ok);

        if (ok == false)
        {
            throw CoreException("Invalid request - the second path parameter in URI must be a cradle ID");
        }

        return id;
    }

private:
    void RouteHandler(const QStringList& pathArguments, IBaseApiView& viewInstance, const QHttpServerRequest& request, QHttpServerResponder& responder);
};

} // namespace dePhonica::Core::Api

#endif // APIENGINE_H
