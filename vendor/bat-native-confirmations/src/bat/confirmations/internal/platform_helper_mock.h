/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_MOCK_H_
#define BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_MOCK_H_

#include <string>

#include "bat/confirmations/internal/platform_helper.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace confirmations {

class PlatformHelperMock : public PlatformHelper {
 public:
  PlatformHelperMock();
  ~PlatformHelperMock() override;

  PlatformHelperMock(const PlatformHelperMock&) = delete;
  PlatformHelperMock& operator=(const PlatformHelperMock&) = delete;

  MOCK_CONST_METHOD0(GetPlatformName, std::string());
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_MOCK_H_
