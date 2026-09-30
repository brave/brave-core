/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_database_table_test_util.h"

#include <utility>

namespace brave_ads::test {

void GetAll(database::table::GetConfirmationTokensCallback callback) {
  database::table::ConfirmationTokens database_table;
  database_table.Load(std::move(callback));
}

}  // namespace brave_ads::test
