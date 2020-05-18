/****
  This file is part of Audio Search Service

  Copyright 2020 Max Klimenko <info@dephonica.com> for dePhonica sound labs.

  NOTICE:  All information contained herein is, and remains the property of dePhonica sound labs
           and its contragents mentioned in related agreements, if any.
****/

#ifndef BASEAPIVIEW_H
#define BASEAPIVIEW_H

#include <QHttpServer>
#include <QJsonObject>
#include <QString>

namespace dePhonica::Core::Api {

class IBaseApiView
{
public:
    virtual QString Name() = 0;
    virtual QStringList Endpoints() = 0;

    virtual QHttpServerRequest::Method MethodsImplemented() = 0;

    virtual QJsonObject Get(const QHttpServerRequest& request, const QStringList& path) = 0;
    virtual QJsonObject Post(const QHttpServerRequest& request, const QStringList& path) = 0;
    virtual QJsonObject Put(const QHttpServerRequest& request, const QStringList& path) = 0;
    virtual QJsonObject Delete(const QHttpServerRequest& request, const QStringList& path) = 0;
};

} // namespace dePhonica::Core::Api

#endif // BASEAPIVIEW_H
