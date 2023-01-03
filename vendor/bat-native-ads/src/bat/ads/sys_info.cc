/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/sys_info.h"

#include "base/no_destructor.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

mojom::SysInfo& SysInfo() {
  static base::NoDestructor<mojom::SysInfo> sys_info;
  return *sys_info;
}

}  // namespace ads
