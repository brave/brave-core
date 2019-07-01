/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/net/proxy_resolution/proxy_resolution_service.h"

#include <utility>

#include "../../../net/proxy_resolution/proxy_resolution_service.cc"  // NOLINT

namespace net {
void ProxyResolutionService::ResetConfigService(
    std::unique_ptr<ProxyConfigService> new_proxy_config_service) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  State previous_state = ResetProxyConfig(true);
  // Release the old configuration service.
  if (config_service_.get())
    config_service_->RemoveObserver(this);
  // Set the new configuration service.
  config_service_ = std::move(new_proxy_config_service);
  config_service_->AddObserver(this);
  if (previous_state != STATE_NONE)
    ApplyProxyConfigIfAvailable();
}
}  // namespace net
