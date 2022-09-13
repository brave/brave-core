/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_PROMOTION_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_PROMOTION_UTIL_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/mojom_structs.h"

namespace ledger {
namespace promotion {

std::string ParseOSToString(mojom::OperatingSystem os);

std::string ParseClientInfoToString(mojom::ClientInfoPtr info);

mojom::PromotionType ConvertStringToPromotionType(const std::string& type);

mojom::ReportType ConvertPromotionTypeToReportType(
    const mojom::PromotionType type);

}  // namespace promotion
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_PROMOTION_UTIL_H_
