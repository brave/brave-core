/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/promotion/promotion_util.h"

namespace ledger {
namespace promotion {

std::string ParseOSToString(type::OperatingSystem os) {
  switch (static_cast<int>(os)) {
    case static_cast<int>(type::OperatingSystem::WINDOWS):  {
      return "windows";
    }
    case static_cast<int>(type::OperatingSystem::MACOS):  {
      return "osx";
    }
    case static_cast<int>(type::OperatingSystem::LINUX):  {
      return "linux";
    }
    case static_cast<int>(type::OperatingSystem::UNDEFINED):  {
      return "undefined";
    }
    default: {
      NOTREACHED();
      return "";
    }
  }
}

std::string ParseClientInfoToString(type::ClientInfoPtr info) {
  if (!info) {
    return "";
  }

  switch (static_cast<int>(info->platform)) {
    case static_cast<int>(type::Platform::ANDROID_R):  {
      return "android";
    }
    case static_cast<int>(type::Platform::IOS):  {
      return "ios";
    }
    case static_cast<int>(type::Platform::DESKTOP):  {
      return ParseOSToString(info->os);
    }
    default: {
      NOTREACHED();
      return "";
    }
  }
}

type::PromotionType ConvertStringToPromotionType(const std::string& type) {
  if (type == "ugp") {
    return type::PromotionType::UGP;
  }

  if (type == "ads") {
    return type::PromotionType::ADS;
  }

  // unknown promotion type, returning dummy value.
  NOTREACHED();
  return type::PromotionType::UGP;
}

type::ReportType ConvertPromotionTypeToReportType(
    const type::PromotionType type) {
  switch (static_cast<int>(type)) {
    case static_cast<int>(type::PromotionType::UGP): {
      return type::ReportType::GRANT_UGP;
    }
    case static_cast<int>(type::PromotionType::ADS): {
      return type::ReportType::GRANT_AD;
    }
    default: {
      NOTREACHED();
      return type::ReportType::GRANT_UGP;
    }
  }
}

}  // namespace promotion
}  // namespace ledger
