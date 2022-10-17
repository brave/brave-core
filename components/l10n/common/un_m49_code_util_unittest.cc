/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/un_m49_code_util.h"

#include "brave/components/l10n/common/un_m49_code_constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=UNM49CodeUtilTest*

namespace brave_l10n {

TEST(UNM49CodeUtilTest, IsUNM49Code) {
  for (const auto& code : kUnM49Codes) {
    EXPECT_TRUE(IsUNM49Code(code));
  }
}

TEST(UNM49CodeUtilTest, IsNotUNM49Code) {
  EXPECT_FALSE(IsUNM49Code("US"));
}

}  // namespace brave_l10n
