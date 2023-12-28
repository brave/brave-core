/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_BIND_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_BIND_UTIL_H_

#include <cstddef>
#include <cstdint>
#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "sql/statement.h"

namespace brave_ads::database {

std::string BuildBindingParameterPlaceholder(size_t parameters_count);
std::string BuildBindingParameterPlaceholders(size_t parameters_count,
                                              size_t binded_parameters_count);

void Bind(sql::Statement* statement,
          const mojom::DBCommandBindingInfo& binding);
void BindNull(mojom::DBCommandInfo* command, int32_t index);
void BindInt(mojom::DBCommandInfo* command, int32_t index, int32_t value);
void BindInt64(mojom::DBCommandInfo* command, int32_t index, int64_t value);
void BindDouble(mojom::DBCommandInfo* command, int32_t index, double value);
void BindBool(mojom::DBCommandInfo* command, int32_t index, bool value);
void BindString(mojom::DBCommandInfo* command,
                int32_t index,
                const std::string& value);

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_BIND_UTIL_H_
