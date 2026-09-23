/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/shutdown_handlers.h"

#include <utility>

#include "base/check.h"

namespace brave_vpn::v2 {

ShutdownHandlers::ShutdownHandlers(ShutdownCallback shutdown_callback)
    : shutdown_callback_(std::move(shutdown_callback)) {
  CHECK(shutdown_callback_);
}

ShutdownHandlers::~ShutdownHandlers() {
  if (installed_) {
    Uninstall();
  }
}

}  // namespace brave_vpn::v2
