/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_history_url_provider.h"

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <string_view>
#include <utility>

#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/test/history_service_test_util.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
#include "components/omnibox/browser/fake_autocomplete_provider_client.h"
#include "components/omnibox/browser/history_quick_provider.h"
#include "components/omnibox/browser/in_memory_url_index.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"
#include "third_party/metrics_proto/omnibox_input_type.pb.h"
#include "url/gurl.h"

class BraveHistoryURLProviderTest : public testing::Test,
                                    public AutocompleteProviderListener {
 public:
  BraveHistoryURLProviderTest() { HistoryQuickProvider::set_disabled(true); }

  ~BraveHistoryURLProviderTest() override {
    HistoryQuickProvider::set_disabled(false);
  }

  BraveHistoryURLProviderTest(const BraveHistoryURLProviderTest&) = delete;
  BraveHistoryURLProviderTest& operator=(const BraveHistoryURLProviderTest&) =
      delete;

  AutocompleteInput CreateAutocompleteInput(std::string_view text) {
    AutocompleteInput input(base::UTF8ToUTF16(text),
                            metrics::OmniboxEventProto::OTHER,
                            TestSchemeClassifier());
    input.set_focus_type(metrics::OmniboxFocusType::INTERACTION_DEFAULT);
    return input;
  }

  // AutocompleteProviderListener:
  void OnProviderUpdate(bool updated_matches,
                        const AutocompleteProvider* provider) override {
    if (autocomplete_->done())
      run_loop_.QuitWhenIdle();
  }

 protected:
  // testing::Test
  void SetUp() override { ASSERT_TRUE(SetUpImpl(true)); }

  void TearDown() override {
    autocomplete_ = nullptr;
    client_.reset();
  }

  // Does the real setup.
  [[nodiscard]] bool SetUpImpl(bool create_history_db) {
    client_ = std::make_unique<FakeAutocompleteProviderClient>();
    auto* registry = static_cast<sync_preferences::TestingPrefServiceSyncable*>(
                         client_->GetPrefs())
                         ->registry();
    omnibox::RegisterBraveProfilePrefs(registry);

    CHECK(history_dir_.CreateUniqueTempDir());
    client_->set_history_service(history::CreateHistoryService(
        history_dir_.GetPath(), create_history_db));
    client_->set_bookmark_model(bookmarks::TestBookmarkClient::CreateModel());
    client_->set_in_memory_url_index(std::make_unique<InMemoryURLIndex>(
        client_->GetBookmarkModel(), client_->GetHistoryService(), nullptr,
        history_dir_.GetPath(), SchemeSet()));
    client_->GetInMemoryURLIndex()->Init();
    if (!client_->GetHistoryService())
      return false;
    autocomplete_ =
        base::MakeRefCounted<BraveHistoryURLProvider>(client_.get(), this);

    client_->GetHistoryService()->AddPageWithDetails(
        GURL("https://example.com"), u"Example", 5, 2, base::Time::Now(), false,
        history::SOURCE_BROWSED);
    return true;
  }

  base::test::TaskEnvironment task_environment_;

  base::RunLoop run_loop_;
  base::ScopedTempDir history_dir_;
  ACMatches matches_;
  std::unique_ptr<FakeAutocompleteProviderClient> client_;
  scoped_refptr<BraveHistoryURLProvider> autocomplete_;
};

TEST_F(BraveHistoryURLProviderTest, NoResultsWhenHistoryDisabled) {
  client_->GetPrefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, false);

  autocomplete_->Start(CreateAutocompleteInput("Example"), false);
  if (!autocomplete_->done())
    run_loop_.Run();
  EXPECT_TRUE(autocomplete_->matches().empty());
}

TEST_F(BraveHistoryURLProviderTest, ResultsWhenHistorySuggestionsEnabled) {
  client_->GetPrefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, true);

  autocomplete_->Start(CreateAutocompleteInput("Example"), false);
  if (!autocomplete_->done())
    run_loop_.Run();
  EXPECT_FALSE(autocomplete_->matches().empty());
}

TEST_F(BraveHistoryURLProviderTest, URLResultsWhenHistoryDisabled) {
  client_->GetPrefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, false);

  autocomplete_->Start(CreateAutocompleteInput("https://unvisited-url.com/"),
                       false);
  if (!autocomplete_->done())
    run_loop_.Run();
  EXPECT_FALSE(autocomplete_->matches().empty());
}
