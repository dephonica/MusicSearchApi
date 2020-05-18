/****
  This file is part of Audio Search Service

  Copyright 2020 Max Klimenko <info@dephonica.com> for dePhonica sound labs.

  NOTICE:  All information contained herein is, and remains the property of dePhonica sound labs
           and its contragents mentioned in related agreements, if any.
****/

#ifndef SESSIONAPIVIEW_H
#define SESSIONAPIVIEW_H

#include <QJsonDocument>
#include <QJsonObject>

#include "CoreException.h"
#include "API/IBaseApiView.h"
#include "API/Session/SessionApiModel.h"

namespace dePhonica::Core::Api {

using namespace dePhonica::Core;
using namespace dePhonica::Core::Interfaces;

class SessionApiView : public IBaseApiView
{
private:
    SessionApiModel sessionModel_;

public:
    SessionApiView(ICoreInstance& coreInstance)
        : sessionModel_(coreInstance)
    {
    }

    QString Name() override { return "SessionApiView"; }
    QStringList Endpoints() override { return { "session", "session/<arg>" }; }

    QHttpServerRequest::Method MethodsImplemented() override { return QHttpServerRequest::Method::All; }

    QJsonObject Get(const QHttpServerRequest&, const QStringList& arguments) override
    {
        if (arguments.size() == 1)
        {
            return sessionModel_.GetSessionInfo(arguments[0]);
        }

        throw CoreException("Invalid GET request - malformed query path");
    }

    QJsonObject Post(const QHttpServerRequest& request, const QStringList& arguments) override
    {
        if (arguments.size() == 0)
        {
            return sessionModel_.CreateSession(QJsonDocument::fromJson(request.body()).object());
        } else if (arguments.size() == 1)
        {
            return sessionModel_.AppendSessionSamples(arguments[0], request.body());
        }

        throw CoreException("Invalid POST request - malformed query path");
    }

    QJsonObject Put(const QHttpServerRequest& request, const QStringList& arguments) override
    {
        if (arguments.size() == 1)
        {
            return sessionModel_.AppendSessionSamples(arguments[0], request.body());
        }

        throw CoreException("Invalid PUT request - malformed query path");
    }

    QJsonObject Delete(const QHttpServerRequest&, const QStringList& arguments) override
    {
        if (arguments.size() == 1)
        {
            return sessionModel_.DeleteSession(arguments[0]);
        }

        throw CoreException("Invalid DELETE request - malformed query path");
    }
};

} // namespace dePhonica::Core::Api

#endif // SESSIONAPIVIEW_H
