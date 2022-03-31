/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/platform/platform_helper_android.h"

#include "base/memory/singleton.h"

namespace ads {

PlatformHelperAndroid::PlatformHelperAndroid() = default;

PlatformHelperAndroid::~PlatformHelperAndroid() = default;

bool PlatformHelperAndroid::IsMobile() const {
  return true;
}

std::string PlatformHelperAndroid::GetPlatformName() const {
  return "android";
}

PlatformType PlatformHelperAndroid::GetPlatform() const {
  return PlatformType::kAndroid;
}

PlatformHelperAndroid* PlatformHelperAndroid::GetInstanceImpl() {
  return base::Singleton<PlatformHelperAndroid>::get();
}

PlatformHelper* PlatformHelper::GetInstanceImpl() {
  return PlatformHelperAndroid::GetInstanceImpl();
}

}  // namespace ads
