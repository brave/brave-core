/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_PROXY_RESOLUTION_PROXY_RESOLUTION_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_NET_PROXY_RESOLUTION_PROXY_RESOLUTION_SERVICE_H_

#include <memory>

// ResetConfigService was removed in c76 in commit
// d44ca309d337c37c778002c13b6ecf5ff6036fdd, but we still need it.
#define BRAVE_PROXY_RESOLUTION_SERVICE_METHODS \
  void ResetConfigService(                     \
      std::unique_ptr<ProxyConfigService> new_proxy_config_service);

#include "../../../net/proxy_resolution/proxy_resolution_service.h"
#undef BRAVE_PROXY_RESOLUTION_SERVICE_METHODS

#endif  // BRAVE_CHROMIUM_SRC_NET_PROXY_RESOLUTION_PROXY_RESOLUTION_SERVICE_H_


