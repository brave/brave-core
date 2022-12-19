/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_store.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

class OmniboxAutocompleteUnitTest : public testing::Test {
 public:
  OmniboxAutocompleteUnitTest() {
    pref_service_ = std::make_unique<TestingPrefServiceSimple>();
    omnibox::RegisterBraveProfilePrefs(pref_service_->registry());
  }

  PrefService* prefs() { return pref_service_.get(); }

 private:
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
};

TEST_F(OmniboxAutocompleteUnitTest, TopSiteSuggestionsEnabledTest) {
  EXPECT_TRUE(prefs()->GetBoolean(omnibox::kTopSiteSuggestionsEnabled));
}
