/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/omnibox/browser/brave_fake_autocomplete_provider_client.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/omnibox/browser/brave_shortcuts_provider.h"
#include "components/history/core/browser/history_service.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/shortcuts_backend.h"
#include "components/omnibox/browser/shortcuts_database.h"
#include "components/omnibox/browser/shortcuts_provider.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"

class MockHistoryService : public history::HistoryService {
 public:
  MockHistoryService() = default;
  MOCK_METHOD1(DeleteURLs, void(const std::vector<GURL>&));
};

class BraveShortcutsProviderTest : public testing::Test {
 public:
  BraveShortcutsProviderTest() = default;

  AutocompleteInput CreateAutocompleteInput(base::StringPiece text) {
    AutocompleteInput input(base::UTF8ToUTF16(text),
                            metrics::OmniboxEventProto::OTHER, classifier_);
    input.set_focus_type(metrics::OmniboxFocusType::INTERACTION_DEFAULT);
    return input;
  }

  void SetUp() override {
    EXPECT_CALL(client_, GetSchemeClassifier())
        .WillRepeatedly(testing::ReturnRef(classifier_));
    EXPECT_CALL(client_, GetHistoryService())
        .WillRepeatedly(testing::Return(&history_service_));

    backend_ = base::MakeRefCounted<ShortcutsBackend>(
        client_.GetTemplateURLService(), std::make_unique<SearchTermsData>(),
        client_.GetHistoryService(), base::FilePath(), true);
    client_.set_shortcuts_backend(backend_);

    backend_->Init();

    ShortcutsDatabase::Shortcut shortcut(
        "BD85DBA2-8C29-49F9-84AE-48E1E90880E9", u"hello",
        ShortcutsDatabase::Shortcut::MatchCore(
            u"hello", GURL("https://example.com"),
            AutocompleteMatch::DocumentType::NONE, u"hello World", "0,0",
            u"hello world", "0,4", ui::PageTransition::PAGE_TRANSITION_TYPED,
            AutocompleteMatch::Type::HISTORY_TITLE, u""),
        base::Time::Now(), 5);
    backend_->AddShortcut(shortcut);

    EXPECT_EQ(1ul, backend_->shortcuts_map().size());

    provider_ = base::MakeRefCounted<BraveShortcutsProvider>(&client_);
  }

  PrefService* prefs() { return client_.GetPrefs(); }

 protected:
  base::test::TaskEnvironment task_environment_;

  MockHistoryService history_service_;
  TestSchemeClassifier classifier_;
  BraveFakeAutocompleteProviderClient client_;
  scoped_refptr<BraveShortcutsProvider> provider_;
  scoped_refptr<ShortcutsBackend> backend_;
};

TEST_F(BraveShortcutsProviderTest, SuggestionsDisabledNoResults) {
  prefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, false);
  provider_->Start(CreateAutocompleteInput("hel"), true);
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(BraveShortcutsProviderTest, SuggestionsEnabledHasResults) {
  prefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, true);
  provider_->Start(CreateAutocompleteInput("hel"), true);
  EXPECT_FALSE(provider_->matches().empty());
}
