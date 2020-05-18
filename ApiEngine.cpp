/****
  This file is part of Audio Search Service

  Copyright 2020 Max Klimenko <info@dephonica.com> for dePhonica sound labs.

  NOTICE:  All information contained herein is, and remains the property of dePhonica sound labs
           and its contragents mentioned in related agreements, if any.
****/

#include "ApiEngine.h"

#include <QJsonDocument>
#include <QMetaEnum>

#include "CoreException.h"

namespace dePhonica::Core::Api {

ApiEngine::ApiEngine(const QString& basePath, quint16 listenPort)
    : basePath_(basePath)
    , listenPort_(listenPort)
{
    if (*(basePath_.end() - 1) != '/')
    {
        basePath_.append('/');
    }
}

ApiEngine::~ApiEngine()
{
}

void ApiEngine::Listen()
{
    httpServer_.listen(QHostAddress::Any, listenPort_);
}

void ApiEngine::RouteHandler(const QStringList& pathArguments,
                             IBaseApiView& viewInstance,
                             const QHttpServerRequest& request,
                             QHttpServerResponder& responder)
{
    QJsonObject resultJson;

    auto isMethodSupported = static_cast<uint32_t>(viewInstance.MethodsImplemented()) & static_cast<uint32_t>(request.method());
    if (!isMethodSupported)
    {
        auto methodName = QMetaEnum::fromType<QHttpServerRequest::Method>().valueToKey(static_cast<int>(request.method()));

        resultJson = QJsonObject(
            { { "result", "error" },
              { "message", QString("Error! HTTP %1 method not implemented for %2").arg(methodName).arg(viewInstance.Name()) } });
    }
    else
    {
        try
        {
            switch (request.method())
            {
            case QHttpServerRequest::Method::Get:
                resultJson = viewInstance.Get(request, pathArguments);
                break;

            case QHttpServerRequest::Method::Post:
                resultJson = viewInstance.Post(request, pathArguments);
                break;

            case QHttpServerRequest::Method::Put:
                resultJson = viewInstance.Put(request, pathArguments);
                break;

            case QHttpServerRequest::Method::Delete:
                resultJson = viewInstance.Delete(request, pathArguments);
                break;

            default:
                break;
            }
        }
        catch (CoreException& ex)
        {
            resultJson = QJsonObject({ { "result", "error" }, { "message", ex.what() } });
        }
    }

    auto statusCode = resultJson.contains("result") && resultJson["result"].toString() == "error"
                          ? QHttpServerResponder::StatusCode::BadRequest
                          : QHttpServerResponder::StatusCode::Ok;

    QJsonDocument doc(resultJson);
    responder.write(doc.toJson(QJsonDocument::Compact), "application/json", statusCode);
}

void ApiEngine::AddEndpoint(IBaseApiView& viewInstance, const QString& overrideUriPath)
{
    QStringList pathCollection(viewInstance.Endpoints());

    if (overrideUriPath.isEmpty() == false)
    {
        pathCollection.clear();
        pathCollection.append(overrideUriPath);
    }

    for (auto& path : pathCollection)
    {
        size_t argCount = path.count("<arg>");

        if (argCount == 0)
        {
            httpServer_.route(basePath_ + path, [this, &viewInstance](const QHttpServerRequest& request, QHttpServerResponder&& responder) {
                QStringList pathArguments;
                RouteHandler(pathArguments, viewInstance, request, responder);
            });
        }
        else if (argCount == 1)
        {
            httpServer_.route(basePath_ + path,
                              [this, &viewInstance](QString arg1, const QHttpServerRequest& request, QHttpServerResponder&& responder) {
                                  QStringList pathArguments;
                                  pathArguments.append(arg1);
                                  RouteHandler(pathArguments, viewInstance, request, responder);
                              });
        }
        else if (argCount == 2)
        {
            httpServer_.route(
                basePath_ + path,
                [this, &viewInstance](QString arg1, QString arg2, const QHttpServerRequest& request, QHttpServerResponder&& responder) {
                    QStringList pathArguments;
                    pathArguments.append(arg1);
                    pathArguments.append(arg2);
                    RouteHandler(pathArguments, viewInstance, request, responder);
                });
        }
        else if (argCount == 3)
        {
            httpServer_.route(basePath_ + path,
                              [this, &viewInstance](QString arg1,
                                                    QString arg2,
                                                    QString arg3,
                                                    const QHttpServerRequest& request,
                                                    QHttpServerResponder&& responder) {
                                  QStringList pathArguments;
                                  pathArguments.append(arg1);
                                  pathArguments.append(arg2);
                                  pathArguments.append(arg3);
                                  RouteHandler(pathArguments, viewInstance, request, responder);
                              });
        }
        else if (argCount == 4)
        {
            httpServer_.route(basePath_ + path,
                              [this, &viewInstance](QString arg1,
                                                    QString arg2,
                                                    QString arg3,
                                                    QString arg4,
                                                    const QHttpServerRequest& request,
                                                    QHttpServerResponder&& responder) {
                                  QStringList pathArguments;
                                  pathArguments.append(arg1);
                                  pathArguments.append(arg2);
                                  pathArguments.append(arg3);
                                  pathArguments.append(arg4);
                                  RouteHandler(pathArguments, viewInstance, request, responder);
                              });
        }
        else if (argCount == 5)
        {
            httpServer_.route(basePath_ + path,
                              [this, &viewInstance](QString arg1,
                                                    QString arg2,
                                                    QString arg3,
                                                    QString arg4,
                                                    QString arg5,
                                                    const QHttpServerRequest& request,
                                                    QHttpServerResponder&& responder) {
                                  QStringList pathArguments;
                                  pathArguments.append(arg1);
                                  pathArguments.append(arg2);
                                  pathArguments.append(arg3);
                                  pathArguments.append(arg4);
                                  pathArguments.append(arg5);
                                  RouteHandler(pathArguments, viewInstance, request, responder);
                              });
        }
    }
}

} // namespace dePhonica::Core::Api
