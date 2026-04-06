/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/platform/test/fake_platform_helper.h"

#include <ostream>  // IWYU pragma: keep
#include <utility>

#include "base/notreached.h"

namespace brave_ads {

namespace {

constexpr char kUnknownPlatformName[] = "unknown";
constexpr char kAndroidPlatformName[] = "android";
constexpr char kIOSPlatformName[] = "ios";
constexpr char kLinuxPlatformName[] = "linux";
constexpr char kMacOSPlatformName[] = "macos";
constexpr char kWindowsPlatformName[] = "windows";

}  // namespace

FakePlatformHelper::FakePlatformHelper() = default;

FakePlatformHelper::~FakePlatformHelper() = default;

void FakePlatformHelper::SetPlatformType(PlatformType type) {
  type_ = type;
}

bool FakePlatformHelper::IsMobile() const {
  return type_ == PlatformType::kAndroid || type_ == PlatformType::kIOS;
}

std::string FakePlatformHelper::GetName() const {
  switch (type_) {
    case PlatformType::kUnknown:
      return kUnknownPlatformName;
    case PlatformType::kAndroid:
      return kAndroidPlatformName;
    case PlatformType::kIOS:
      return kIOSPlatformName;
    case PlatformType::kLinux:
      return kLinuxPlatformName;
    case PlatformType::kMacOS:
      return kMacOSPlatformName;
    case PlatformType::kWindows:
      return kWindowsPlatformName;
  }
  NOTREACHED() << "Unexpected value for PlatformType: "
               << std::to_underlying(type_);
}

PlatformType FakePlatformHelper::GetType() const {
  return type_;
}

}  // namespace brave_ads
