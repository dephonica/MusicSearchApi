/****
  This file is part of Audio Search Service

  Copyright 2020 Max Klimenko <info@dephonica.com> for dePhonica sound labs.

  NOTICE:  All information contained herein is, and remains the property of dePhonica sound labs
           and its contragents mentioned in related agreements, if any.
****/

#include "ApiInstance.h"

namespace dePhonica::Core::Api {

ApiInstance::ApiInstance(QString baseUri, quint16 listenPort, ICoreInstance &coreInstance)
    : coreInstance_(coreInstance)
    , sessionView_(coreInstance)
    , apiEngine_(baseUri, listenPort)
{
}

void ApiInstance::Start()
{
    apiEngine_.AddEndpoint(versionView_);
    apiEngine_.AddEndpoint(sessionView_);

    apiEngine_.Listen();
}

} // namespace dePhonica::Core::Api
