/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <regex>
#include <sstream>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "bat/ledger/internal/core/future.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=FutureTest.*

namespace ledger {

class FutureTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(FutureTest, MultipleValues1) {
  int int_value = 0;
  char char_value = '0';
  double double_value = 0.0;

  Promise<int, char, double> promise;
  auto future = promise.GetFuture();
  future.Then(base::BindLambdaForTesting([&](int iv, char cv, double dv) {
    int_value = iv;
    char_value = cv;
    double_value = dv;
  }));
  promise.Set(5, '5', 5.0);

  EXPECT_EQ(int_value, 0);
  EXPECT_EQ(char_value, '0');
  EXPECT_EQ(double_value, 0.0);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(int_value, 5);
  EXPECT_EQ(char_value, '5');
  EXPECT_EQ(double_value, 5.0);
}

TEST_F(FutureTest, MultipleValues2) {
  int int_value = 0;
  char char_value = '0';
  double double_value = 0.0;

  Promise<int, char, double> promise;

  promise | base::BindLambdaForTesting([&](int iv, char cv, double dv) {
    int_value = iv;
    char_value = cv;
    double_value = dv;
  });

  promise.Set(5, '5', 5.0);

  EXPECT_EQ(int_value, 0);
  EXPECT_EQ(char_value, '0');
  EXPECT_EQ(double_value, 0.0);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(int_value, 5);
  EXPECT_EQ(char_value, '5');
  EXPECT_EQ(double_value, 5.0);
}

TEST_F(FutureTest, ValueSentInFutureTurn) {
  int value = 0;
  MakeFuture(10).Then(
      base::BindLambdaForTesting([&value](int v) { value = v; }));
  EXPECT_EQ(value, 0);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(value, 10);
}

TEST_F(FutureTest, CompleteCallbacksExecutedInFutureTurn) {
  Promise<int> promise;
  int value = 0;
  promise.GetFuture().Then(
      base::BindLambdaForTesting([&value](int v) { value = v; }));
  promise.Set(1);
  EXPECT_EQ(value, 0);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(value, 1);
}

TEST_F(FutureTest, TransformingThen1) {
  double value = 0;

  MakeFuture(1)
      .Then(base::BindOnce([](int v) { return static_cast<double>(v) / 2; }))
      .Then(base::BindLambdaForTesting([&value](double v) { value = v; }));

  EXPECT_EQ(value, 0);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(value, 0.5);
}

TEST_F(FutureTest, TransformingThen2) {
  double value = 0;

  MakeFuture(1)
      .Then(base::BindOnce(
          [](int v) { return std::make_tuple(static_cast<double>(v) / 2); }))
      .Then(base::BindLambdaForTesting([&value](double v) { value = v; }));

  EXPECT_EQ(value, 0);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(value, 0.5);
}

TEST_F(FutureTest, TransformingThen3) {
  double double_value = 0.0;
  std::string string_value;

  MakeFuture(1)
      .Then(base::BindOnce([](int v) {
        return std::make_tuple(static_cast<double>(v) / 2,
                               std ::to_string(v) + " / 2");
      }))
      .Then(base::BindLambdaForTesting([&](double dv, std::string sv) {
        double_value = dv;
        string_value = std::move(sv);
      }));

  EXPECT_EQ(double_value, 0.0);
  EXPECT_TRUE(string_value.empty());
  task_environment_.RunUntilIdle();
  EXPECT_EQ(double_value, 0.5);
  EXPECT_EQ(string_value, "1 / 2");
}

TEST_F(FutureTest, UnwrappingThen1) {
  bool value = false;

  MakeFuture(42)
      .Then(base::BindOnce([](int value) { return MakeFuture(true); }))
      .Then(base::BindLambdaForTesting([&value](bool v) { value = v; }));

  EXPECT_FALSE(value);
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(value);
}

TEST_F(FutureTest, UnwrappingThen2) {
  double double_value = 0.0;
  std::string string_value;

  MakeFuture(1)
      .Then(base::BindOnce([](int v) {
        return MakeFuture(static_cast<double>(v) / 2,
                          std ::to_string(v) + " / 2");
      }))
      .Then(base::BindLambdaForTesting([&](double dv, std::string sv) {
        double_value = dv;
        string_value = std::move(sv);
      }));

  EXPECT_EQ(double_value, 0.0);
  EXPECT_TRUE(string_value.empty());
  task_environment_.RunUntilIdle();
  EXPECT_EQ(double_value, 0.5);
  EXPECT_EQ(string_value, "1 / 2");
}

TEST_F(FutureTest, DiscardValueThen1) {
  bool called = false;
  MakeFuture(1).Then(
      base::BindLambdaForTesting([&called]() { called = true; }));
  EXPECT_FALSE(called);
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(called);
}

TEST_F(FutureTest, DiscardValueThen2) {
  bool called1 = false;
  bool called2 = false;
  MakeFuture(1)
      .Then(base::BindOnce([] { return 'c'; }))
      .Then(base::BindLambdaForTesting([&](char) {
        called1 = true;
        return 8;
      }))
      .Then(base::BindOnce([](int i) { return ++i; }))
      .Then(base::BindLambdaForTesting([&] { called2 = true; }));
  EXPECT_FALSE(called1);
  EXPECT_FALSE(called2);
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(called1);
  EXPECT_TRUE(called2);
}

TEST_F(FutureTest, MakeFuture) {
  int value = 0;
  MakeFuture(1).Then(
      base::BindLambdaForTesting([&value](int v) { value = v; }));
  EXPECT_EQ(value, 0);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(value, 1);
}

// clang-format off
// transforming `Then` -
// base::OnceCallback<std::tuple<std::string::const_iterator, std::string::const_iterator, std::regex>>(const std::string&)>
// clang-format on
auto prepare_tokenize_input = [](const std::string& s) {
  return std::tuple(
      s.cbegin(), s.cend(),
      std::regex{"[A-Z]+[a-z]+"} /* regex for matching camel case */);
};

// clang-format off
// transforming `Then` -
// base::OnceCallback<std::vector<std::string>(std::string::const_iterator, std::string::const_iterator, std::regex)>
// clang-format on
auto tokenize = [](std::string::const_iterator cbegin,
                   std::string::const_iterator cend,
                   std::regex regex) {
  return std::vector<std::string>(
      std::sregex_token_iterator(cbegin, cend, regex),
      std::sregex_token_iterator());
};

// transforming `Then` -
// base::OnceCallback<std::string(std::vector<std::string>)>
auto join_with_underscore = [](std::vector<std::string> tokens) {
  EXPECT_TRUE(tokens.size() == 4);
  EXPECT_EQ(tokens[0], "Camel");
  EXPECT_EQ(tokens[1], "Case");
  EXPECT_EQ(tokens[2], "Is");
  EXPECT_EQ(tokens[3], "Gone");

  std::ostringstream oss;

  auto cend = tokens.cend();
  std::copy(tokens.cbegin(), --cend,
            std::ostream_iterator<std::string>(oss, "_"));

  return oss.str() + *cend;
};

// transforming/flattening `Then` -
// base::OnceCallback<Future<std::string>(std::string)>
auto lower_case = [](std::string s) {
  EXPECT_EQ(s, "Camel_Case_Is_Gone");

  Promise<std::string> promise;
  auto future = promise.GetFuture();

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](Promise<std::string> promise, std::string s) {
                       std::transform(
                           s.cbegin(), s.cend(), s.begin(), [](char c) {
                             return static_cast<char>(
                                 std::tolower(static_cast<unsigned char>(c)));
                           });

                       promise.Set(std::move(s));
                     },
                     std::move(promise), std::move(s)));

  return future;
};

TEST_F(FutureTest, CamelCaseToSnakeCase1) {
  std::string camel_case = "CamelCaseIsGone";
  std::string snake_case;

  Promise<const std::string&> promise;

  promise.GetFuture()
      .Then(base::BindOnce(prepare_tokenize_input))
      .Then(base::BindOnce(tokenize))
      .Then(base::BindOnce(join_with_underscore))
      .Then(base::BindOnce(lower_case))
      .Then(base::BindLambdaForTesting(
          [&](std::string s) { snake_case = std::move(s); }))
      .Then(base::BindLambdaForTesting(
          [&] { EXPECT_EQ(snake_case, "camel_case_is_gone"); }));

  promise.Set(std::cref(camel_case));

  task_environment_.RunUntilIdle();
}

TEST_F(FutureTest, CamelCaseToSnakeCase2) {
  std::string camel_case = "CamelCaseIsGone";
  std::string snake_case;

  Promise<const std::string&> promise;
  // clang-format off
  promise
    | prepare_tokenize_input
    | tokenize
    | join_with_underscore
    | lower_case
    | base::BindLambdaForTesting([&](std::string s) { snake_case = std::move(s); })
    | base::BindLambdaForTesting([&] { EXPECT_EQ(snake_case, "camel_case_is_gone"); })
  ;
  // clang-format on
  promise.Set(std::cref(camel_case));

  task_environment_.RunUntilIdle();
}

TEST_F(FutureTest, UnrelatedTasks1) {
  std::string s1, s2;

  Promise<> promise;
  promise.GetFuture()
      .Then(base::BindLambdaForTesting([&] { s1 = "unrelated"; }))
      .Then(base::BindLambdaForTesting([&] { s2 = "tasks"; }));
  promise.Set();

  task_environment_.RunUntilIdle();
  EXPECT_EQ(s1, "unrelated");
  EXPECT_EQ(s2, "tasks");
}

TEST_F(FutureTest, UnrelatedTasks2) {
  std::string s1, s2;
  int i1 = 0;

  Promise<> promise;
  promise.GetFuture()
      .Then(base::BindLambdaForTesting([&] { s1 = "unrelated"; }))
      .Then(base::BindOnce([] { return 42; }))
      .Then(base::BindLambdaForTesting([&](int i2) { i1 = i2; }))
      .Then(base::BindLambdaForTesting([&] { s2 = "tasks"; }));
  promise.Set();

  task_environment_.RunUntilIdle();
  EXPECT_EQ(s1, "unrelated");
  EXPECT_EQ(i1, 42);
  EXPECT_EQ(s2, "tasks");
}

TEST_F(FutureTest, UnrelatedTasks3) {
  std::string s1, s2;
  int i1 = 0;

  Promise<int> promise;
  promise.GetFuture()
      .Then(base::BindLambdaForTesting([&](int i2) { i1 = i2; }))
      .Then(base::BindLambdaForTesting([&] { s1 = "unrelated"; }))
      .Then(base::BindLambdaForTesting([&] { s2 = "tasks"; }));
  promise.Set(42);

  task_environment_.RunUntilIdle();
  EXPECT_EQ(i1, 42);
  EXPECT_EQ(s1, "unrelated");
  EXPECT_EQ(s2, "tasks");
}

}  // namespace ledger
