/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/helper/helper_app.h"

#include "base/logging.h"

namespace brave_vpn {
namespace v2 {

HelperApp::HelperApp() = default;

HelperApp::~HelperApp() = default;

int HelperApp::Run() {
  VLOG(1) << "Hello from the Brave VPN Helper!";
  return 0;
}

}  // namespace v2
}  // namespace brave_vpn
