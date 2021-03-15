/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/bat_ledger_context.h"

#include <string>

#include "bat/ledger/internal/core/async_result.h"
#include "bat/ledger/internal/core/bat_ledger_test.h"

namespace ledger {

class BATLedgerContextTest : public BATLedgerTest {};

struct TestSettings {
  std::string a;
  int b;

  static const TestSettings kProduction;
  static const TestSettings kStaging;
  static const TestSettings kDevelopment;
};

const TestSettings TestSettings::kProduction = {.a = "prod", .b = 1};
const TestSettings TestSettings::kStaging = {.a = "staging", .b = 2};
const TestSettings TestSettings::kDevelopment = {.a = "dev", .b = 3};

TEST_F(BATLedgerContextTest, GetSettings) {
  struct ResetEnv {
    ResetEnv() : env(ledger::_environment) {}
    ~ResetEnv() { ledger::_environment = env; }
    mojom::Environment env;
  } reset_env;

  ASSERT_EQ(ledger::_environment, mojom::Environment::PRODUCTION);
  ledger::_environment = mojom::Environment::PRODUCTION;
  auto& prod = context()->GetSettings<TestSettings>();
  ASSERT_EQ(prod.a, "prod");
  ASSERT_EQ(prod.b, 1);

  ledger::_environment = mojom::Environment::STAGING;
  auto& staging = context()->GetSettings<TestSettings>();
  ASSERT_EQ(staging.a, "staging");
  ASSERT_EQ(staging.b, 2);

  ledger::_environment = mojom::Environment::DEVELOPMENT;
  auto& dev = context()->GetSettings<TestSettings>();
  ASSERT_EQ(dev.a, "dev");
  ASSERT_EQ(dev.b, 3);
}

class TestComponent : public BATLedgerContext::Component {
 public:
  static const BATLedgerContext::ComponentKey kComponentKey;
  explicit TestComponent(BATLedgerContext* context) : Component(context) {}
};

const BATLedgerContext::ComponentKey TestComponent::kComponentKey;

TEST_F(BATLedgerContextTest, GetComponent) {
  auto* component = context()->Get<TestComponent>();
  EXPECT_TRUE(component);
  EXPECT_EQ(context()->Get<TestComponent>(), component);
}

TEST_F(BATLedgerContextTest, StartTask) {
  class Task : public BATLedgerContext::Component {
   public:
    explicit Task(BATLedgerContext* context) : Component(context) {}
    AsyncResult<int> result() const { return resolver_.result(); }
    void Start(int n) { resolver_.Complete(std::move(n)); }

   private:
    AsyncResult<int>::Resolver resolver_;
  };

  int value = 0;
  context()->StartTask<Task>(100).Then(
      base::BindLambdaForTesting([&value](const int& v) { value = v; }));

  task_environment()->RunUntilIdle();
  EXPECT_EQ(value, 100);
}

}  // namespace ledger
