/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_MOCK_H_

#include "bat/ads/internal/common/platform/platform_helper.h"

#include <string>

#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

namespace ads {

class PlatformHelperMock : public PlatformHelper {
 public:
  PlatformHelperMock();

  PlatformHelperMock(const PlatformHelperMock& other) = delete;
  PlatformHelperMock& operator=(const PlatformHelperMock& other) = delete;

  PlatformHelperMock(PlatformHelperMock&& other) noexcept = delete;
  PlatformHelperMock& operator=(PlatformHelperMock&& other) noexcept = delete;

  ~PlatformHelperMock() override;

  MOCK_CONST_METHOD0(IsMobile, bool());
  MOCK_CONST_METHOD0(GetName, std::string());
  MOCK_CONST_METHOD0(GetType, PlatformType());
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_MOCK_H_
