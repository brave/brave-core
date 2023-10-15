/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_NETWORK_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_NETWORK_SERVICE_H_

#include "services/network/public/mojom/network_service.mojom.h"

#define DisableQuic                                                         \
  GetDnsRequestCountsAndReset(GetDnsRequestCountsAndResetCallback callback) \
      override;                                                             \
  void DisableQuic

#include "src/services/network/network_service.h"  // IWYU pragma: export

#undef DisableQuic

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_NETWORK_SERVICE_H_
