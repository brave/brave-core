/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/mutated_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsMutatedUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsMutatedUserDataTest, BuildMutatedConfirmationsUserData) {
  // Arrange
  SetDefaultUint64Pref(
      prefs::kConfirmationsHash,
      /*core/test/data/test/confirmations.json has a hash of 3780921521*/
      1251290873);

  // Act

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(R"({"mutated":true})"),
            BuildMutatedUserData());
}

TEST_F(BraveAdsMutatedUserDataTest, BuildMutatedClientUserData) {
  // Arrange
  SetDefaultUint64Pref(
      prefs::kClientHash,
      /*core/test/data/test/client.json has a hash of 2810715844*/ 4485170182);

  // Act

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(R"({"mutated":true})"),
            BuildMutatedUserData());
}

}  // namespace brave_ads
