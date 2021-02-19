/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SECURITY_INTERSTITIALS_CORE_CONTROLLER_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SECURITY_INTERSTITIALS_CORE_CONTROLLER_CLIENT_H_

#define GetBaseHelpCenterUrl                 \
  GetBaseHelpCenterUrl_ChromiumImpl() const; \
  GURL GetBaseHelpCenterUrl

#include "../../../../../components/security_interstitials/core/controller_client.h"
#undef GetBaseHelpCenterUrl

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SECURITY_INTERSTITIALS_CORE_CONTROLLER_CLIENT_H_
