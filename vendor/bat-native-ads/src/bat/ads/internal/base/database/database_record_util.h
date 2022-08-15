/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_DATABASE_DATABASE_RECORD_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_DATABASE_DATABASE_RECORD_UTIL_H_

#include <vector>

#include "bat/ads/public/interfaces/ads.mojom.h"
#include "sql/statement.h"

namespace ads {
namespace database {

mojom::DBRecordInfoPtr CreateRecord(
    sql::Statement* statement,
    const std::vector<mojom::DBCommandInfo::RecordBindingType>& bindings);

}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_DATABASE_DATABASE_RECORD_UTIL_H_
