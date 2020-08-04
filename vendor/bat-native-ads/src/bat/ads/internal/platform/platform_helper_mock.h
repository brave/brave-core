/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_PLATFORM_PLATFORM_HELPER_MOCK_H_
#define BAT_ADS_INTERNAL_PLATFORM_PLATFORM_HELPER_MOCK_H_

#include <string>

#include "testing/gmock/include/gmock/gmock.h"
#include "bat/ads/internal/platform/platform_helper.h"

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

#endif  // BAT_ADS_INTERNAL_PLATFORM_PLATFORM_HELPER_MOCK_H_
