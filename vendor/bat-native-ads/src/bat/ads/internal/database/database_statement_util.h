/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_DATABASE_DATABASE_STATEMENT_UTIL_H_
#define BAT_ADS_INTERNAL_DATABASE_DATABASE_STATEMENT_UTIL_H_

#include <stdint.h>

#include <string>

#include "bat/ads/mojom.h"

namespace ads {
namespace database {

std::string BuildBindingParameterPlaceholder(
    const size_t parameters_count);

std::string BuildBindingParameterPlaceholders(
    const size_t parameters_count,
    const size_t values_count);

void BindNull(
    DBCommand* command,
    const int index);

void BindInt(
    DBCommand* command,
    const int index,
    const int32_t value);

void BindInt64(
    DBCommand* command,
    const int index,
    const int64_t value);

void BindDouble(
    DBCommand* command,
    const int index,
    const double value);

void BindBool(
    DBCommand* command,
    const int index,
    const bool value);

void BindString(
    DBCommand* command,
    const int index,
    const std::string& value);

int ColumnInt(
    DBRecord* record,
    const size_t index);

int64_t ColumnInt64(
    DBRecord* record,
    const size_t index);

double ColumnDouble(
    DBRecord* record,
    const size_t index);

bool ColumnBool(
    DBRecord* record,
    const size_t index);

std::string ColumnString(
    DBRecord* record,
    const size_t index);

}  // namespace database
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_DATABASE_DATABASE_STATEMENT_UTIL_H_
