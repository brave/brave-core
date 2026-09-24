/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_android.h"

namespace brave_ads {

namespace {
constexpr char kOperatingSystemName[] = "android";
}  // namespace

OperatingSystemAndroid::OperatingSystemAndroid() = default;

std::string OperatingSystemAndroid::GetName() const {
  return kOperatingSystemName;
}

OperatingSystemType OperatingSystemAndroid::GetType() const {
  return OperatingSystemType::kAndroid;
}

}  // namespace brave_ads
