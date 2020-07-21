/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_UTIL_H_
#define BRAVELEDGER_DATABASE_DATABASE_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"
#include "sql/database.h"

namespace braveledger_database {

const size_t kBatchLimit = 999;

void BindNull(
    ledger::DBCommand* command,
    const int index);

void BindInt(
    ledger::DBCommand* command,
    const int index,
    const int32_t value);

void BindInt64(
    ledger::DBCommand* command,
    const int index,
    const int64_t value);

void BindDouble(
    ledger::DBCommand* command,
    const int index,
    const double value);

void BindBool(
    ledger::DBCommand* command,
    const int index,
    const bool value);

void BindString(
    ledger::DBCommand* command,
    const int index,
    const std::string& value);

int32_t GetCurrentVersion();

int32_t GetCompatibleVersion();

void OnResultCallback(
    ledger::DBCommandResponsePtr response,
    ledger::ResultCallback callback);

int GetIntColumn(ledger::DBRecord* record, const int index);

int64_t GetInt64Column(ledger::DBRecord* record, const int index);

double GetDoubleColumn(ledger::DBRecord* record, const int index);

bool GetBoolColumn(ledger::DBRecord* record, const int index);

std::string GetStringColumn(ledger::DBRecord* record, const int index);

std::string GenerateStringInCase(const std::vector<std::string>& items);

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_UTIL_H_
