/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/promotion/promotion_util.h"

namespace ledger {
namespace promotion {

std::string ParseOSToString(mojom::OperatingSystem os) {
  switch (static_cast<int>(os)) {
    case static_cast<int>(mojom::OperatingSystem::WINDOWS): {
      return "windows";
    }
    case static_cast<int>(mojom::OperatingSystem::MACOS): {
      return "osx";
    }
    case static_cast<int>(mojom::OperatingSystem::LINUX): {
      return "linux";
    }
    case static_cast<int>(mojom::OperatingSystem::UNDEFINED): {
      return "undefined";
    }
    default: {
      NOTREACHED();
      return "";
    }
  }
}

std::string ParseClientInfoToString(mojom::ClientInfoPtr info) {
  if (!info) {
    return "";
  }

  switch (static_cast<int>(info->platform)) {
    case static_cast<int>(mojom::Platform::ANDROID_R): {
      return "android";
    }
    case static_cast<int>(mojom::Platform::IOS): {
      return "ios";
    }
    case static_cast<int>(mojom::Platform::DESKTOP): {
      return ParseOSToString(info->os);
    }
    default: {
      NOTREACHED();
      return "";
    }
  }
}

mojom::PromotionType ConvertStringToPromotionType(const std::string& type) {
  if (type == "ugp") {
    return mojom::PromotionType::UGP;
  }

  if (type == "ads") {
    return mojom::PromotionType::ADS;
  }

  // unknown promotion type, returning dummy value.
  NOTREACHED();
  return mojom::PromotionType::UGP;
}

mojom::ReportType ConvertPromotionTypeToReportType(
    const mojom::PromotionType type) {
  switch (static_cast<int>(type)) {
    case static_cast<int>(mojom::PromotionType::UGP): {
      return mojom::ReportType::GRANT_UGP;
    }
    case static_cast<int>(mojom::PromotionType::ADS): {
      return mojom::ReportType::GRANT_AD;
    }
    default: {
      NOTREACHED();
      return mojom::ReportType::GRANT_UGP;
    }
  }
}

}  // namespace promotion
}  // namespace ledger
