/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/country_code_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/l10n/common/country_code_feature.h"
#include "brave/components/l10n/common/prefs.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=CountryCodeUtilTest*

namespace brave_l10n {

class CountryCodeUtilTest : public ::testing::Test {
 public:
  CountryCodeUtilTest() {
    RegisterL10nLocalStatePrefs(local_state_.registry());
  }

  PrefService* local_state() { return &local_state_; }

 private:
  test::ScopedDefaultLocale scoped_default_locale_{"en_CA"};
  TestingPrefServiceSimple local_state_;
};

TEST_F(CountryCodeUtilTest, GetDefaultCountryCodeFromLocalState) {
  EXPECT_EQ("CA", GetCountryCode(local_state()));
}

TEST_F(CountryCodeUtilTest, GetCountryCodeFromLocalState) {
  local_state()->Set(prefs::kCountryCode, base::Value("GB"));
  EXPECT_EQ("GB", GetCountryCode(local_state()));
}

TEST_F(CountryCodeUtilTest, GetCountryCodeCodeAsDefaultISOCountryCode) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(kFetchResourcesByCountryCodeFeature);
  local_state()->Set(prefs::kCountryCode, base::Value("KY"));

  EXPECT_EQ("CA", GetCountryCode(local_state()));
}

}  // namespace brave_l10n
