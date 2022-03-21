/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/bat_ledger_initializer.h"

#include <string>

#include "bat/ledger/internal/core/bat_ledger_test.h"

namespace ledger {

class BATLedgerInitializerTest : public BATLedgerTest {};

TEST_F(BATLedgerInitializerTest, InitializeIsIdempotent) {
  std::string last_message;
  auto on_log = [&](const std::string& message) { last_message = message; };
  SetLogCallbackForTesting(base::BindLambdaForTesting(on_log));

  // The initializer is assumed to generate logging messages. Use this property
  // to determine whether additional calls to |Initialize| have side-effects.
  context().Get<BATLedgerInitializer>().Initialize();
  task_environment()->RunUntilIdle();
  EXPECT_FALSE(last_message.empty());
  last_message = "";

  context().Get<BATLedgerInitializer>().Initialize();
  task_environment()->RunUntilIdle();
  EXPECT_TRUE(last_message.empty());
}

}  // namespace ledger
