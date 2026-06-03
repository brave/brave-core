/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/task_environment.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_match_test_util.h"
#include "components/omnibox/browser/fake_autocomplete_controller.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_store.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

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

TEST_F(OmniboxAutocompleteUnitTest, OnDeviceSuggestionsEnabledTest) {
  EXPECT_TRUE(prefs()->GetBoolean(omnibox::kOnDeviceSuggestionsEnabled));
}

// Brave overrides UpdateMatchDestinationURLWithInvocationSource to be a no-op,
// suppressing Chrome's source attribution params (chrome.ob / chrome.rb) which
// have no meaning outside Google's infrastructure and clobber Brave Search's
// own source param.
class InvocationSourceTest : public testing::Test {
 protected:
  base::test::SingleThreadTaskEnvironment task_environment_;
  FakeAutocompleteController controller_{&task_environment_};
};

TEST_F(InvocationSourceTest, NotInjectedFromOmnibox) {
  controller_.input_ = AutocompleteInput(
      u"a", 1u, metrics::OmniboxEventProto::NTP, TestSchemeClassifier());

  AutocompleteMatch match = CreateSearchMatch(u"search term");
  match.destination_url =
      GURL("https://search.brave.com/search?q=search+term&source=desktop");

  controller_.UpdateMatchDestinationURLWithInvocationSource(&match);

  EXPECT_EQ(match.destination_url.spec(),
            "https://search.brave.com/search?q=search+term&source=desktop");
}

TEST_F(InvocationSourceTest, NotInjectedFromRealbox) {
  controller_.input_ =
      AutocompleteInput(u"a", 1u, metrics::OmniboxEventProto::NTP_REALBOX,
                        TestSchemeClassifier());

  AutocompleteMatch match = CreateSearchMatch(u"search term");
  match.destination_url =
      GURL("https://search.brave.com/search?q=search+term&source=desktop");

  controller_.UpdateMatchDestinationURLWithInvocationSource(&match);

  EXPECT_EQ(match.destination_url.spec(),
            "https://search.brave.com/search?q=search+term&source=desktop");
}

TEST_F(InvocationSourceTest, NotInjectedForThirdPartyEngines) {
  controller_.input_ = AutocompleteInput(
      u"a", 1u, metrics::OmniboxEventProto::NTP, TestSchemeClassifier());

  AutocompleteMatch match = CreateSearchMatch(u"search term");
  match.destination_url = GURL("https://google.com/search?q=search+term");

  controller_.UpdateMatchDestinationURLWithInvocationSource(&match);

  EXPECT_EQ(match.destination_url.spec(),
            "https://google.com/search?q=search+term");
}
