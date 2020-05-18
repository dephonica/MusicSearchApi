/****
  This file is part of Audio Search Service

  Copyright 2020 Max Klimenko <info@dephonica.com> for dePhonica sound labs.

  NOTICE:  All information contained herein is, and remains the property of dePhonica sound labs
           and its contragents mentioned in related agreements, if any.
****/

#ifndef VERSIONAPIMODEL_H
#define VERSIONAPIMODEL_H

#include <QJsonObject>
#include <QString>

namespace dePhonica::Core::Api {

class VersionApiModel
{
public:
    VersionApiModel();

    QString ProductName;
    QString SoftwareVersion;
    QString HardwareVersion;

    QJsonObject ToJson()
    {
        return { { "ProductName", ProductName },
                 { "SoftwareVersion", SoftwareVersion },
                 { "HardwareVersion", HardwareVersion },
                 { "result", "ok" } };
    }
};

} // namespace dePhonica::Core::Api

#endif // VERSIONAPIMODEL_H
