/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/public/cpp/third_party_service_launcher.h"

namespace brave_wallet {

// A global instance which controls if ThirdPartyService will be launched
// out-of-process or in process.
ThirdPartyServiceLauncher* g_instance = nullptr;

// static
ThirdPartyServiceLauncher* ThirdPartyServiceLauncher::GetInstance() {
  return g_instance;
}

ThirdPartyServiceLauncher::ThirdPartyServiceLauncher() {
  CHECK(!g_instance);
  g_instance = this;
}

ThirdPartyServiceLauncher::~ThirdPartyServiceLauncher() {
  CHECK(g_instance);
  g_instance = nullptr;
}

}  // namespace brave_wallet
