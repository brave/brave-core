/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "sql/database.h"

namespace brave_rewards::internal {
namespace database {

const size_t kBatchLimit = 999;

void BindNull(mojom::DBCommand* command, const int index);

void BindInt(mojom::DBCommand* command, const int index, const int32_t value);

void BindInt64(mojom::DBCommand* command, const int index, const int64_t value);

void BindDouble(mojom::DBCommand* command, const int index, const double value);

void BindBool(mojom::DBCommand* command, const int index, const bool value);

void BindString(mojom::DBCommand* command,
                const int index,
                const std::string& value);

int32_t GetCurrentVersion();

int32_t GetCompatibleVersion();

void OnResultCallback(mojom::DBCommandResponsePtr response,
                      LegacyResultCallback callback);

int GetIntColumn(mojom::DBRecord* record, const int index);

int64_t GetInt64Column(mojom::DBRecord* record, const int index);

double GetDoubleColumn(mojom::DBRecord* record, const int index);

bool GetBoolColumn(mojom::DBRecord* record, const int index);

std::string GetStringColumn(mojom::DBRecord* record, const int index);

std::string GenerateStringInCase(const std::vector<std::string>& items);

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_UTIL_H_
