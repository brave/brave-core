/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_PLATFORM_TEST_FAKE_PLATFORM_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_PLATFORM_TEST_FAKE_PLATFORM_HELPER_H_

#include <string>

#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_types.h"

namespace brave_ads {

// A test double for `PlatformHelper` that returns a fixed platform type. Use
// `SetForTesting` to install it, then call `SetPlatformType` to change the
// simulated platform at any point during a test.
class FakePlatformHelper : public PlatformHelper {
 public:
  FakePlatformHelper();

  FakePlatformHelper(const FakePlatformHelper&) = delete;
  FakePlatformHelper& operator=(const FakePlatformHelper&) = delete;

  ~FakePlatformHelper() override;

  void SetPlatformType(PlatformType type);

  // PlatformHelper:
  bool IsMobile() const override;
  std::string GetName() const override;
  PlatformType GetType() const override;

 private:
  PlatformType type_ = PlatformType::kUnknown;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_PLATFORM_TEST_FAKE_PLATFORM_HELPER_H_
