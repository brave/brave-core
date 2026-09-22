// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"
#include "chrome/browser/history_embeddings/history_embeddings_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
#include "components/omnibox/browser/history_embeddings_provider.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"

namespace history_embeddings {

namespace {

class TestProviderListener : public AutocompleteProviderListener {
 public:
  void OnProviderUpdate(bool updated_matches,
                        const AutocompleteProvider* provider) override {}
};

}  // namespace

// The setting is live, but the service is built from it once at profile setup,
// so the two can disagree.
class HistoryEmbeddingsOmniboxBrowserTest : public InProcessBrowserTest {
 public:
  HistoryEmbeddingsOmniboxBrowserTest() {
    feature_list_.InitAndEnableFeature(kHistoryEmbeddings);
  }

  void SetSemanticHistorySearchEnabled(Profile* profile, bool enabled) {
    profile->GetPrefs()->SetBoolean(
        local_ai::prefs::kBraveHistoryEmbeddingsEnabled, enabled);
  }

  bool IsEnabledForOmnibox(Profile* profile) {
    return ChromeAutocompleteProviderClient(profile)
        .IsHistoryEmbeddingsEnabled();
  }

  // Enough words to get past `search_query_minimum_word_count`. The provider
  // runs in @history keyword mode, the only scope it has while
  // `omnibox_unscoped` is off.
  void StartHistoryEmbeddingsProvider(Profile* profile) {
    auto client = std::make_unique<ChromeAutocompleteProviderClient>(profile);
    TestProviderListener listener;
    auto provider = base::MakeRefCounted<HistoryEmbeddingsProvider>(
        client.get(), &listener);

    TestSchemeClassifier scheme_classifier;
    AutocompleteInput input(u"semantic history search",
                            metrics::OmniboxEventProto::OTHER,
                            scheme_classifier);
    provider->Start(input, /*minimal_changes=*/false);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Setting turned on after profile setup, so no service was built for it.
IN_PROC_BROWSER_TEST_F(HistoryEmbeddingsOmniboxBrowserTest,
                       NotEnabledForOmniboxUntilRelaunch) {
  Profile* profile = browser()->GetProfile();
  ASSERT_FALSE(profile->GetPrefs()->GetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled));

  SetSemanticHistorySearchEnabled(profile, true);

  // What the brave://history toggle shows.
  EXPECT_TRUE(IsHistoryEmbeddingsEnabledForProfile(profile));
  ASSERT_FALSE(HistoryEmbeddingsServiceFactory::GetForProfile(profile));
  EXPECT_FALSE(IsEnabledForOmnibox(profile));

  StartHistoryEmbeddingsProvider(profile);
}

// Leaves the setting on for the run below.
IN_PROC_BROWSER_TEST_F(HistoryEmbeddingsOmniboxBrowserTest,
                       PRE_EnabledForOmniboxAfterRelaunch) {
  SetSemanticHistorySearchEnabled(browser()->GetProfile(), true);
}

// Setting on at profile setup, so the service is built for it.
IN_PROC_BROWSER_TEST_F(HistoryEmbeddingsOmniboxBrowserTest,
                       EnabledForOmniboxAfterRelaunch) {
  Profile* profile = browser()->GetProfile();
  ASSERT_TRUE(profile->GetPrefs()->GetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled));

  ASSERT_TRUE(HistoryEmbeddingsServiceFactory::GetForProfile(profile));
  EXPECT_TRUE(IsEnabledForOmnibox(profile));

  StartHistoryEmbeddingsProvider(profile);
}

}  // namespace history_embeddings
