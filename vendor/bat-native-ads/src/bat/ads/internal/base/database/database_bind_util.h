/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_DATABASE_DATABASE_BIND_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_DATABASE_DATABASE_BIND_UTIL_H_

#include <cstdint>
#include <string>

#include "bat/ads/public/interfaces/ads.mojom.h"
#include "sql/statement.h"

namespace ads {
namespace database {

std::string BuildBindingParameterPlaceholder(const size_t parameters_count);
std::string BuildBindingParameterPlaceholders(const size_t parameters_count,
                                              const size_t values_count);

void Bind(sql::Statement* statement, const mojom::DBCommandBinding& binding);
void BindNull(mojom::DBCommand* command, const int index);
void BindInt(mojom::DBCommand* command, const int index, const int32_t value);
void BindInt64(mojom::DBCommand* command, const int index, const int64_t value);
void BindDouble(mojom::DBCommand* command, const int index, const double value);
void BindBool(mojom::DBCommand* command, const int index, const bool value);
void BindString(mojom::DBCommand* command,
                const int index,
                const std::string& value);

}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_DATABASE_DATABASE_BIND_UTIL_H_
