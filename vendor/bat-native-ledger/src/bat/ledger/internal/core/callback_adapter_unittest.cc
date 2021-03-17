/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/callback_adapter.h"

#include <vector>

#include "bat/ledger/internal/core/bat_ledger_test.h"

namespace ledger {

class CallbackAdapterTest : public BATLedgerTest {};

TEST_F(CallbackAdapterTest, WrapLambda) {
  class A {
   private:
    auto MakeLambda() {
      return [this](int value) { results_->push_back(value); };
    }

   public:
    explicit A(std::vector<int>* results) : results_(results) {
      callback_adapter_(MakeLambda()).Run(1);
    }

    ~A() { callback_adapter_(MakeLambda()).Run(2); }

    base::OnceCallback<void(int)> AdaptLambda() {
      return callback_adapter_(MakeLambda());
    }

   private:
    CallbackAdapter callback_adapter_;
    std::vector<int>* results_;
  };

  base::OnceCallback<void(int)> callback;
  std::vector<int> results;

  {
    A a(&results);
    a.AdaptLambda().Run(3);
    callback = a.AdaptLambda();
  }

  std::move(callback).Run(4);

  EXPECT_EQ(results.size(), 3ul);
  EXPECT_EQ(results[0], 1);
  EXPECT_EQ(results[1], 3);
  EXPECT_EQ(results[2], 2);
}

TEST_F(CallbackAdapterTest, ResultCode) {
  EXPECT_EQ(CallbackAdapter::ResultCode(true), mojom::Result::LEDGER_OK);
  EXPECT_EQ(CallbackAdapter::ResultCode(false), mojom::Result::LEDGER_ERROR);
}

}  // namespace ledger
