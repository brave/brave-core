/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/publisher/publisher_data.h"

namespace ledger {

Publisher::Publisher() = default;
Publisher::~Publisher() = default;

Publisher::Publisher(const Publisher& other) = default;
Publisher& Publisher::operator=(const Publisher& other) = default;

Publisher::Publisher(Publisher&& other) = default;
Publisher& Publisher::operator=(Publisher&& other) = default;

}  // namespace ledger
