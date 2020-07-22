/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/promotion/promotion_util.h"

namespace braveledger_promotion {

std::string ParseOSToString(ledger::OperatingSystem os) {
  switch (static_cast<int>(os)) {
    case static_cast<int>(ledger::OperatingSystem::WINDOWS):  {
      return "windows";
    }
    case static_cast<int>(ledger::OperatingSystem::MACOS):  {
      return "osx";
    }
    case static_cast<int>(ledger::OperatingSystem::LINUX):  {
      return "linux";
    }
    case static_cast<int>(ledger::OperatingSystem::UNDEFINED):  {
      return "undefined";
    }
    default: {
      NOTREACHED();
      return "";
    }
  }
}

std::string ParseClientInfoToString(ledger::ClientInfoPtr info) {
  if (!info) {
    return "";
  }

  switch (static_cast<int>(info->platform)) {
    case static_cast<int>(ledger::Platform::ANDROID_R):  {
      return "android";
    }
    case static_cast<int>(ledger::Platform::IOS):  {
      return "ios";
    }
    case static_cast<int>(ledger::Platform::DESKTOP):  {
      return ParseOSToString(info->os);
    }
    default: {
      NOTREACHED();
      return "";
    }
  }
}

ledger::PromotionType ConvertStringToPromotionType(const std::string& type) {
  if (type == "ugp") {
    return ledger::PromotionType::UGP;
  }

  if (type == "ads") {
    return ledger::PromotionType::ADS;
  }

  // unknown promotion type, returning dummy value.
  NOTREACHED();
  return ledger::PromotionType::UGP;
}

ledger::ReportType ConvertPromotionTypeToReportType(
    const ledger::PromotionType type) {
  switch (static_cast<int>(type)) {
    case static_cast<int>(ledger::PromotionType::UGP): {
      return ledger::ReportType::GRANT_UGP;
    }
    case static_cast<int>(ledger::PromotionType::ADS): {
      return ledger::ReportType::GRANT_AD;
    }
    default: {
      NOTREACHED();
      return ledger::ReportType::GRANT_UGP;
    }
  }
}

std::vector<ledger::PromotionType> GetEligiblePromotions() {
  return {
    ledger::PromotionType::ADS
  };
}

}  // namespace braveledger_promotion
