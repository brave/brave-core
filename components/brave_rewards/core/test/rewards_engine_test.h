/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_REWARDS_ENGINE_TEST_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_REWARDS_ENGINE_TEST_H_

#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/test/test_rewards_engine_client.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_rewards::internal {

// Base class for unit tests. |RewardsEngineTest| provides a task environment
// and a test implementation of |mojom::RewardsEngineClient|.
class RewardsEngineTest : public testing::Test {
 public:
  RewardsEngineTest();

  explicit RewardsEngineTest(std::unique_ptr<TestRewardsEngineClient> client);

  ~RewardsEngineTest() override;

  void InitializeEngine();

 protected:
  // Returns the |TaskEnvironment| for this test.
  base::test::TaskEnvironment& task_environment() { return task_environment_; }

  // Returns the |TestRewardsEngineClient| instance for this test.
  TestRewardsEngineClient& client() { return *client_; }

  // Returns the |RewardsEngine| instance for this test.
  RewardsEngine& engine() { return engine_; }

  // Executes the supplied lambda with a zero-arg callback, waits until the
  // callback has been executed, and then returns control to the caller.
  template <typename F>
  void WaitFor(F fn) {
    base::RunLoop run_loop;
    fn(base::BindLambdaForTesting([&run_loop] { run_loop.Quit(); }));
    run_loop.Run();
  }

  // Executes the supplied lambda with a callback that accepts a value of the
  // specified type, waits until the callback has been executed, and then
  // returns the value to the caller.
  template <typename T, typename F>
  std::decay_t<T> WaitFor(F fn) {
    base::RunLoop run_loop;
    std::optional<std::decay_t<T>> result;
    fn(base::BindLambdaForTesting([&result, &run_loop](T arg) {
      result = std::decay_t<T>(std::move(arg));
      run_loop.Quit();
    }));
    run_loop.Run();
    return std::move(*result);
  }

  // Executes the supplied lambda with an appropriate callback, waits until the
  // callback has been executed, and then returns the values to the caller as a
  // tuple.
  template <typename... Args, typename F>
  std::tuple<std::decay_t<Args>...> WaitForValues(F fn) {
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
  std::unique_ptr<TestRewardsEngineClient> client_;
  mojo::AssociatedReceiver<mojom::RewardsEngineClient> client_receiver_;
  RewardsEngine engine_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_REWARDS_ENGINE_TEST_H_
