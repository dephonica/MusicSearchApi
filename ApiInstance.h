/****
  This file is part of Audio Search Service

  Copyright 2020 Max Klimenko <info@dephonica.com> for dePhonica sound labs.

  NOTICE:  All information contained herein is, and remains the property of dePhonica sound labs
           and its contragents mentioned in related agreements, if any.
****/

#ifndef APIINSTANCE_H
#define APIINSTANCE_H

#include "API/ApiEngine.h"
#include "CoreInstance.h"

#include "API/Version/VersionApiView.h"
#include "API/Session/SessionApiView.h"

namespace dePhonica::Core::Api {

using namespace dePhonica::Core::Interfaces;

class ApiInstance
{
private:
    ICoreInstance& coreInstance_;

    VersionApiView versionView_;
    SessionApiView sessionView_;

    ApiEngine apiEngine_;

public:
    ApiInstance(QString baseUri, quint16 listenPort, ICoreInstance& coreInstance);

    void Start();
};

} // namespace dePhonica::Core::Api

#endif // APIINSTANCE_H
