/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_TEST_SUPPORT_IN_PROCESS_THIRD_PARTY_SERVICE_TEST_ENVIRONMENT_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_TEST_SUPPORT_IN_PROCESS_THIRD_PARTY_SERVICE_TEST_ENVIRONMENT_H_

#include <memory>

namespace brave_wallet {

class ThirdPartyServiceLauncher;

class InProcessThirdPartyServiceTestEnvironment {
 public:
  InProcessThirdPartyServiceTestEnvironment();
  InProcessThirdPartyServiceTestEnvironment(
      const InProcessThirdPartyServiceTestEnvironment& other) = delete;
  InProcessThirdPartyServiceTestEnvironment& operator=(
      const InProcessThirdPartyServiceTestEnvironment& other) = delete;
  virtual ~InProcessThirdPartyServiceTestEnvironment();

 private:
  std::unique_ptr<ThirdPartyServiceLauncher> third_party_service_launcher_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_TEST_SUPPORT_IN_PROCESS_THIRD_PARTY_SERVICE_TEST_ENVIRONMENT_H_
