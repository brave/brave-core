// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/history_embeddings/history_embeddings_service_factory.h"
#include "brave/browser/history_embeddings/history_embeddings_utils.h"
#include "brave/components/history_embeddings/core/history_embeddings_features.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "brave/components/omnibox/browser/brave_history_embeddings_provider.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/omnibox/omnibox_controller.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/location_bar/selected_keyword_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
#include "components/omnibox/browser/omnibox_client.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/omnibox/browser/vector_icons.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_starter_pack_data.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/base/ui_base_features.h"

namespace brave_history_embeddings {

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
               .GetBraveHistoryEmbeddingsSearch() != nullptr;
  }

  // Enough words to get past `search_query_minimum_word_count`. The provider
  // runs in @history keyword mode, the only scope it has while
  // `omnibox_unscoped` is off.
  void StartHistoryEmbeddingsProvider(Profile* profile) {
    auto client = std::make_unique<ChromeAutocompleteProviderClient>(profile);
    TestProviderListener listener;
    auto provider = base::MakeRefCounted<BraveHistoryEmbeddingsProvider>(
        client.get(), &listener);

    TestSchemeClassifier scheme_classifier;
    AutocompleteInput input(u"semantic history search",
                            metrics::OmniboxEventProto::OTHER,
                            scheme_classifier);
    provider->Start(input, /*minimal_changes=*/false);
  }

  OmniboxController* omnibox_controller() {
    return BrowserView::GetBrowserViewForBrowser(browser())
        ->GetLocationBarView()
        ->GetOmniboxController();
  }

  // The @history starter pack engine.
  const TemplateURL* HistoryStarterPackEngine() {
    TemplateURLService* service =
        TemplateURLServiceFactory::GetForProfile(browser()->GetProfile());
    search_test_utils::WaitForTemplateURLServiceToLoad(service);
    return service->FindStarterPackTemplateURL(
        template_url_starter_pack_data::StarterPackId::kHistory);
  }

  // The icon the keyword chip shows for the @history scope.
  const gfx::VectorIcon* HistoryKeywordIcon() {
    const TemplateURL* history = HistoryStarterPackEngine();
    ui::ImageModel icon = SelectedKeywordView::GetKeywordIcon(
        history->keyword(), omnibox_controller(), browser()->GetProfile());
    return icon.IsVectorIcon() ? icon.GetVectorIcon().vector_icon() : nullptr;
  }

  const gfx::VectorIcon* SparkIcon() {
    return features::IsRoundedIconsEnabled() ? &omnibox::kSearchSparkIcon
                                             : &omnibox::kSearchSparkOldIcon;
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

// The @history scope shows the semantic search placeholder and spark icon only
// while the setting is on.
IN_PROC_BROWSER_TEST_F(HistoryEmbeddingsOmniboxBrowserTest,
                       HistoryScopePlaceholderAndIcon) {
  Profile* profile = browser()->GetProfile();
  const TemplateURL* history = HistoryStarterPackEngine();
  ASSERT_TRUE(history);

  ASSERT_FALSE(omnibox_controller()->client()->IsHistoryEmbeddingsEnabled());
  EXPECT_EQ(
      AutocompleteMatch::GetKeywordPlaceholder(history, false),
      l10n_util::GetStringUTF16(IDS_OMNIBOX_HISTORY_SCOPE_PLACEHOLDER_TEXT));
  EXPECT_NE(HistoryKeywordIcon(), SparkIcon());

  SetSemanticHistorySearchEnabled(profile, true);

  EXPECT_TRUE(omnibox_controller()->client()->IsHistoryEmbeddingsEnabled());
  EXPECT_EQ(AutocompleteMatch::GetKeywordPlaceholder(
                history,
                omnibox_controller()->client()->IsHistoryEmbeddingsEnabled()),
            l10n_util::GetStringUTF16(
                IDS_OMNIBOX_HISTORY_EMBEDDINGS_SCOPE_PLACEHOLDER_TEXT));
  EXPECT_EQ(HistoryKeywordIcon(), SparkIcon());
}

}  // namespace brave_history_embeddings
