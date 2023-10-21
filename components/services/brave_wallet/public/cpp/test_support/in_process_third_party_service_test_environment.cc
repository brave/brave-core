/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/public/cpp/test_support/in_process_third_party_service_test_environment.h"

#include <memory>

#include "brave/components/services/brave_wallet/in_process_third_party_service_launcher.h"
#include "brave/components/services/brave_wallet/public/cpp/third_party_service.h"

namespace brave_wallet {

InProcessThirdPartyServiceTestEnvironment::
    InProcessThirdPartyServiceTestEnvironment() {
  third_party_service_launcher_ =
      std::make_unique<InProcessThirdPartyServiceLauncher>();
}

InProcessThirdPartyServiceTestEnvironment::
    ~InProcessThirdPartyServiceTestEnvironment() {
  ThirdPartyService::Get().ResetForTesting();
}

}  // namespace brave_wallet
