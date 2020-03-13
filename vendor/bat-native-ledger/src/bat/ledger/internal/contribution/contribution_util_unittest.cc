/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/values.h"
#include "bat/ledger/internal/contribution/contribution_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=ContributionUtilTest.*

namespace braveledger_contribution {

class ContributionUtilTest : public testing::Test {};

TEST_F(ContributionUtilTest, GenerateSuggestionEmptyToken) {
  base::Value token(base::Value::Type::DICTIONARY);
  bool success = GenerateSuggestion(
      "",
      "rqQ1Tz26C4mv33ld7xpcLhuX1sWaD+s7VMnuX6cokT4=",
      "some id",
      &token);
  ASSERT_FALSE(success);
}

TEST_F(ContributionUtilTest, GenerateSuggestionEmptyPublicKey) {
  base::Value token(base::Value::Type::DICTIONARY);
  bool success = GenerateSuggestion(
      "Twsi4QeUNcOOW/IFnYcGLHgLsfVco0oPZ+cl3YeZMQgb4NB8E29Ts+2/pQA54VksqGpg/DtmPRBV4FlQ1VXbKmiwO9BI7jDXSN33CCb+rSBe1PCG1LtigUOfzaIVF3c5",  // NOLINT
      "",
      "some id",
      &token);
  ASSERT_FALSE(success);
}

TEST_F(ContributionUtilTest, GenerateSuggestionEmptyPayload) {
  base::Value token(base::Value::Type::DICTIONARY);
  bool success = GenerateSuggestion(
      "Twsi4QeUNcOOW/IFnYcGLHgLsfVco0oPZ+cl3YeZMQgb4NB8E29Ts+2/pQA54VksqGpg/DtmPRBV4FlQ1VXbKmiwO9BI7jDXSN33CCb+rSBe1PCG1LtigUOfzaIVF3c5",  // NOLINT
      "rqQ1Tz26C4mv33ld7xpcLhuX1sWaD+s7VMnuX6cokT4=",
      "",
      &token);
  ASSERT_FALSE(success);
}

TEST_F(ContributionUtilTest, GenerateSuggestionGenerated) {
  const std::string public_key =
      "rqQ1Tz26C4mv33ld7xpcLhuX1sWaD+s7VMnuX6cokT4=";
  base::Value token(base::Value::Type::DICTIONARY);
  bool success = GenerateSuggestion(
      "Twsi4QeUNcOOW/IFnYcGLHgLsfVco0oPZ+cl3YeZMQgb4NB8E29Ts+2/pQA54VksqGpg/DtmPRBV4FlQ1VXbKmiwO9BI7jDXSN33CCb+rSBe1PCG1LtigUOfzaIVF3c5",  // NOLINT
      public_key,
      "some id",
      &token);
  ASSERT_TRUE(success);
  ASSERT_EQ(*token.FindStringKey("t"), "Twsi4QeUNcOOW/IFnYcGLHgLsfVco0oPZ+cl3YeZMQgb4NB8E29Ts+2/pQA54VksqGpg/DtmPRBV4FlQ1VXbKg==");  // NOLINT
  ASSERT_EQ(*token.FindStringKey("publicKey"), public_key);
  ASSERT_EQ(*token.FindStringKey("signature"), "qnQvRh1dWoc/YKAGVYgP4PljOoK10T8ryMqLGY6RFc3Gig8mZCzmuGH5IQtVtCZ0x42/pOFKfX3rUpzIL4wPUw==");  // NOLINT
}

}  // namespace braveledger_contribution
