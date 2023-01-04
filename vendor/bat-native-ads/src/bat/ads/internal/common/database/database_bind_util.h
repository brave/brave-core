/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_DATABASE_DATABASE_BIND_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_DATABASE_DATABASE_BIND_UTIL_H_

#include <cstdint>
#include <string>

#include "bat/ads/public/interfaces/ads.mojom-forward.h"
#include "sql/statement.h"

namespace ads::database {

std::string BuildBindingParameterPlaceholder(size_t parameters_count);
std::string BuildBindingParameterPlaceholders(size_t parameters_count,
                                              size_t values_count);

void Bind(sql::Statement* statement,
          const mojom::DBCommandBindingInfo& binding);
void BindNull(mojom::DBCommandInfo* command, int index);
void BindInt(mojom::DBCommandInfo* command, int index, int32_t value);
void BindInt64(mojom::DBCommandInfo* command, int index, int64_t value);
void BindDouble(mojom::DBCommandInfo* command, int index, double value);
void BindBool(mojom::DBCommandInfo* command, int index, bool value);
void BindString(mojom::DBCommandInfo* command,
                int index,
                const std::string& value);

}  // namespace ads::database

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_DATABASE_DATABASE_BIND_UTIL_H_
