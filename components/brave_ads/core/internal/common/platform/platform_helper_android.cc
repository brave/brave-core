/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_android.h"

namespace brave_ads {

namespace {
constexpr char kPlatformName[] = "android";
}  // namespace

PlatformHelperAndroid::PlatformHelperAndroid() = default;

bool PlatformHelperAndroid::IsMobile() const {
  return true;
}

std::string PlatformHelperAndroid::GetName() const {
  return kPlatformName;
}

PlatformType PlatformHelperAndroid::GetType() const {
  return PlatformType::kAndroid;
}

}  // namespace brave_ads
