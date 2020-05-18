/****
  This file is part of Audio Search Service

  Copyright 2020 Max Klimenko <info@dephonica.com> for dePhonica sound labs.

  NOTICE:  All information contained herein is, and remains the property of dePhonica sound labs
           and its contragents mentioned in related agreements, if any.
****/

#include "VersionApiModel.h"

namespace dePhonica::Core::Api {

VersionApiModel::VersionApiModel()
{
    ProductName = "dePhonica audio search service";
    SoftwareVersion = "2.1.9";
    HardwareVersion = "1.0.0";
}

} // namespace dePhonica::Core::Api
