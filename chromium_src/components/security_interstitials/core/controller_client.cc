/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/security_interstitials/core/controller_client.h"

#define GetBaseHelpCenterUrl GetBaseHelpCenterUrl_ChromiumImpl
#include "../../../../../components/security_interstitials/core/controller_client.cc"
#undef GetBaseHelpCenterUrl

namespace security_interstitials {

GURL ControllerClient::GetBaseHelpCenterUrl() const {
  return GURL("https://support.brave.com/");
}

}  // namespace security_interstitials
