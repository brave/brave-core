/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/password_manager/core/browser/ui/weak_check_utility_unittest.cc"

#include <string>
#include <utility>

namespace password_manager {

using ParamType = std::pair<std::string,  // password
                            int           // expected strength
                            >;

class WeakCheckUtilityTest : public testing::TestWithParam<ParamType> {};

TEST_P(WeakCheckUtilityTest, GetPasswordStrength) {
  EXPECT_EQ(GetPasswordStrength(std::get<0>(GetParam())),
            std::get<1>(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(
    WeakCheckUtilityTest,
    WeakCheckUtilityTest,
    testing::Values(ParamType{"", 0},
                    ParamType{base::UTF16ToUTF8(kWeakShortPassword), 20},
                    ParamType{base::UTF16ToUTF8(kWeakLongPassword), 20},
                    ParamType{base::UTF16ToUTF8(kStrongShortPassword), 100},
                    ParamType{base::UTF16ToUTF8(kStrongLongPassword), 100}));

}  // namespace password_manager
