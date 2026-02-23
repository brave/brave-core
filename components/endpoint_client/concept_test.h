/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CONCEPT_TEST_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CONCEPT_TEST_H_

#include <string>
#include <tuple>

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoint_client {

struct ConceptTest {
  template <typename T>
  struct Fixture : testing::Test {
    using TestType = std::tuple_element_t<0, T>;
    using ExpectedResult = std::tuple_element_t<1, T>;
  };

  struct NameGenerator {
    template <typename T>
    static std::string GetName(int) {
      using TestType = std::tuple_element_t<0, T>;
      using ExpectedResult = std::tuple_element_t<1, T>;
      return std::string(TestType::kName) + "_does" +
             (ExpectedResult::value ? "" : "_not");
    }
  };
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CONCEPT_TEST_H_
