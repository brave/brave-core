/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SYS_INFO_HELPER_MOCK_H_
#define BAT_ADS_INTERNAL_SYS_INFO_HELPER_MOCK_H_

#include "bat/ads/internal/sys_info_helper.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace ads {

class SysInfoHelperMock : public SysInfoHelper {
 public:
  SysInfoHelperMock();
  ~SysInfoHelperMock() override;

  SysInfoHelperMock(const SysInfoHelperMock&) = delete;
  SysInfoHelperMock& operator=(const SysInfoHelperMock&) = delete;

  void Initialize() override;

  MOCK_CONST_METHOD0(GetHardware, base::SysInfo::HardwareInfo());
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SYS_INFO_HELPER_MOCK_H_
