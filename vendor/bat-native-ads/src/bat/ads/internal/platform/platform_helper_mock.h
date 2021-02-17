/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PLATFORM_PLATFORM_HELPER_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PLATFORM_PLATFORM_HELPER_MOCK_H_

#include <string>

#include "bat/ads/internal/platform/platform_helper.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ads {

class PlatformHelperMock : public PlatformHelper {
 public:
  PlatformHelperMock();
  ~PlatformHelperMock() override;

  PlatformHelperMock(const PlatformHelperMock&) = delete;
  PlatformHelperMock& operator=(const PlatformHelperMock&) = delete;

  MOCK_CONST_METHOD0(IsMobile, bool());
  MOCK_CONST_METHOD0(GetPlatformName, std::string());
  MOCK_CONST_METHOD0(GetPlatform, PlatformType());
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PLATFORM_PLATFORM_HELPER_MOCK_H_
