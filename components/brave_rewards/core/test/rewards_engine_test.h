/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_REWARDS_ENGINE_TEST_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_REWARDS_ENGINE_TEST_H_

#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/test/test_rewards_engine_client.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_rewards::internal {

// Base class for unit tests. |RewardsEngineTest| provides a task environment
// and a test implementation of |mojom::RewardsEngineClient|.
class RewardsEngineTest : public testing::Test {
 public:
  RewardsEngineTest();
  ~RewardsEngineTest() override;

  void InitializeEngine();

 protected:
  // Returns the |TaskEnvironment| for this test.
  base::test::TaskEnvironment& task_environment() { return task_environment_; }

  // Returns the |TestRewardsEngineClient| instance for this test.
  TestRewardsEngineClient& engine_client() { return client_; }

  // Returns the |RewardsEngineImpl| instance for this test.
  RewardsEngineImpl& engine() { return engine_; }

  // Adds a mock network response for the specified URL and HTTP method.
  void AddNetworkResultForTesting(const std::string& url,
                                  mojom::UrlMethod method,
                                  mojom::UrlResponsePtr response);

  // Sets a callback that is executed when a message is logged to the client.
  void SetLogCallbackForTesting(TestRewardsEngineClient::LogCallback callback);

  // Executes the supplied lambda with an appropriate callback, waits until the
  // callback has been executed, and the returns the values to the caller as a
  // tuple.
  template <typename... Args, typename F>
  std::tuple<std::decay_t<Args>...> WaitFor(F fn) {
    base::RunLoop run_loop;
    std::optional<std::tuple<std::decay_t<Args>...>> result;
    fn(base::BindLambdaForTesting([&result, &run_loop](Args... args) {
      result = std::tuple<std::decay_t<Args>...>(std::move(args)...);
      run_loop.Quit();
    }));
    run_loop.Run();
    return std::move(*result);
  }

 private:
  base::test::TaskEnvironment task_environment_;
  TestRewardsEngineClient client_;
  mojo::AssociatedReceiver<mojom::RewardsEngineClient> client_receiver_{
      &client_};
  RewardsEngineImpl engine_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_REWARDS_ENGINE_TEST_H_
