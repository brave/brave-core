/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/sys_info_helper_mock.h"

namespace ads {

SysInfoHelperMock::SysInfoHelperMock() = default;

SysInfoHelperMock::~SysInfoHelperMock() = default;

void SysInfoHelperMock::Initialize() {
  if (ready_.is_signaled()) {
    return;
  }

  ready_.Signal();
}

}  // namespace ads
