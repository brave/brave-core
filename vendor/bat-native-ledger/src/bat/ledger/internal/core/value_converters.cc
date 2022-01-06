/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/value_converters.h"

namespace ledger {

ValueReader::ValueReader(const base::Value& value) : value_(value) {}

ValueReader::~ValueReader() = default;

ValueWriter::ValueWriter() = default;

ValueWriter::~ValueWriter() = default;

}  // namespace ledger
