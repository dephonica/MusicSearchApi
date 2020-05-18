/****
  This file is part of Audio Search Service

  Copyright 2020 Max Klimenko <info@dephonica.com> for dePhonica sound labs.

  NOTICE:  All information contained herein is, and remains the property of dePhonica sound labs
           and its contragents mentioned in related agreements, if any.
****/

#ifndef VERSIONAPIVIEW_H
#define VERSIONAPIVIEW_H

#include <QJsonDocument>
#include <QJsonObject>

#include "API/IBaseApiView.h"
#include "API/Version/VersionApiModel.h"

namespace dePhonica::Core::Api {

class VersionApiView : public IBaseApiView
{
private:
    VersionApiModel versionModel_;

public:
    VersionApiView();

    QString Name() override { return "VersionApiView"; }
    QStringList Endpoints() override { return {"version"}; }

    QHttpServerRequest::Method MethodsImplemented() override { return QHttpServerRequest::Method::Get; }

    QJsonObject Get(const QHttpServerRequest&, const QStringList&) override { return versionModel_.ToJson(); }

    QJsonObject Post(const QHttpServerRequest&, const QStringList&) override { return QJsonObject(); }
    QJsonObject Put(const QHttpServerRequest&, const QStringList&) override { return QJsonObject(); }
    QJsonObject Delete(const QHttpServerRequest&, const QStringList&) override { return QJsonObject(); }
};

} // namespace dePhonica::Core::Api

#endif // VERSIONAPIVIEW_H
