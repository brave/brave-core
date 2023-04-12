/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_MOCK_H_

#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"

#include <string>

#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class PlatformHelperMock : public PlatformHelper {
 public:
  PlatformHelperMock();

  PlatformHelperMock(const PlatformHelperMock&) = delete;
  PlatformHelperMock& operator=(const PlatformHelperMock&) = delete;

  PlatformHelperMock(PlatformHelperMock&&) noexcept = delete;
  PlatformHelperMock& operator=(PlatformHelperMock&&) noexcept = delete;

  ~PlatformHelperMock() override;

  MOCK_CONST_METHOD0(IsMobile, bool());
  MOCK_CONST_METHOD0(GetName, std::string());
  MOCK_CONST_METHOD0(GetType, PlatformType());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_MOCK_H_
