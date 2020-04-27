/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/platform_helper_android.h"

namespace confirmations {

PlatformHelperAndroid::PlatformHelperAndroid() = default;

PlatformHelperAndroid::~PlatformHelperAndroid() = default;

std::string PlatformHelperAndroid::GetPlatformName() const {
  return "android";
}

PlatformHelperAndroid* PlatformHelperAndroid::GetInstanceImpl() {
  return base::Singleton<PlatformHelperAndroid>::get();
}

PlatformHelper* PlatformHelper::GetInstanceImpl() {
  return PlatformHelperAndroid::GetInstanceImpl();
}

}  // namespace confirmations
