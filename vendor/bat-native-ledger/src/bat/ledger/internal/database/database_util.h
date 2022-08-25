/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"
#include "sql/database.h"

namespace ledger {
namespace database {

const size_t kBatchLimit = 999;

void BindNull(
    type::DBCommand* command,
    const int index);

void BindInt(
    type::DBCommand* command,
    const int index,
    const int32_t value);

void BindInt64(
    type::DBCommand* command,
    const int index,
    const int64_t value);

void BindDouble(
    type::DBCommand* command,
    const int index,
    const double value);

void BindBool(
    type::DBCommand* command,
    const int index,
    const bool value);

void BindString(
    type::DBCommand* command,
    const int index,
    const std::string& value);

int32_t GetCurrentVersion();

int32_t GetCompatibleVersion();

template <typename>
inline constexpr bool dependent_false_v = false;

template <typename ResultCallback = ledger::ResultCallback>
void OnResultCallback(ResultCallback callback,
                      type::DBCommandResponsePtr response) {
  const bool success =
      response &&
      response->status == type::DBCommandResponse::Status::RESPONSE_OK;
  if constexpr (std::is_same_v<ResultCallback, ledger::LegacyResultCallback>) {
    callback(success ? type::Result::LEDGER_OK : type::Result::LEDGER_ERROR);
  } else if constexpr (std::is_same_v<ResultCallback, ledger::ResultCallback>) {
    std::move(callback).Run(success ? type::Result::LEDGER_OK
                                    : type::Result::LEDGER_ERROR);
  } else {
    static_assert(dependent_false_v<ResultCallback>,
                  "ResultCallback must be either "
                  "ledger::LegacyResultCallback, or "
                  "ledger::ResultCallback!");
  }
}

int GetIntColumn(type::DBRecord* record, const int index);

int64_t GetInt64Column(type::DBRecord* record, const int index);

double GetDoubleColumn(type::DBRecord* record, const int index);

bool GetBoolColumn(type::DBRecord* record, const int index);

std::string GetStringColumn(type::DBRecord* record, const int index);

std::string GenerateStringInCase(const std::vector<std::string>& items);

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_UTIL_H_
