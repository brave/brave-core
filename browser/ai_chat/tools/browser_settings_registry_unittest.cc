// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/browser_settings_registry.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat::browser_settings {

namespace {

std::vector<std::string_view> SearchIds(std::string_view query,
                                        size_t max_results = 10) {
  std::vector<std::string_view> ids;
  for (const auto& match : SearchPrefs(query, max_results)) {
    ids.push_back(match.pref_name);
  }
  return ids;
}

}  // namespace

class BrowserSettingsRegistryTest : public testing::Test {
 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
};

// The allowlist is a compile-time table of preference path strings, so nothing
// stops an entry from naming a preference that was renamed, removed, is
// registered in local state rather than the profile, or is a "generated"
// settings_private pseudo-preference that never existed in PrefService. All of
// those would make the value tool report the setting as unavailable forever,
// so assert the whole table resolves against a real profile PrefService.
TEST_F(BrowserSettingsRegistryTest, EveryEntryIsARegisteredProfilePref) {
  PrefService* prefs = profile_.GetPrefs();
  ASSERT_FALSE(GetAllowedPrefs().empty());
  for (std::string_view pref_name : GetAllowedPrefs()) {
    EXPECT_TRUE(prefs->FindPreference(pref_name))
        << "Allowlisted '" << pref_name
        << "' is not a registered profile preference.";
  }
}

// Dictionaries and lists hold structured user data (site lists, URLs, device
// records) rather than a value the user picked in the settings UI, so they must
// never reach the allowlist. The value tool also refuses them at read time.
TEST_F(BrowserSettingsRegistryTest, EveryEntryIsScalarValued) {
  PrefService* prefs = profile_.GetPrefs();
  for (std::string_view pref_name : GetAllowedPrefs()) {
    const PrefService::Preference* pref = prefs->FindPreference(pref_name);
    if (!pref) {
      continue;  // Covered by EveryEntryIsARegisteredProfilePref.
    }
    EXPECT_FALSE(pref->GetValue()->is_dict())
        << pref_name << " is a dictionary";
    EXPECT_FALSE(pref->GetValue()->is_list()) << pref_name << " is a list";
  }
}

// IsAllowedPref() binary searches, so sortedness is load-bearing rather than
// merely tidy. Uniqueness keeps search results from repeating an entry.
TEST_F(BrowserSettingsRegistryTest, EntriesAreSortedAndUnique) {
  EXPECT_TRUE(std::ranges::is_sorted(GetAllowedPrefs()));
  EXPECT_TRUE(std::ranges::adjacent_find(GetAllowedPrefs()) ==
              GetAllowedPrefs().end());
}

TEST_F(BrowserSettingsRegistryTest, IsAllowedPref) {
  EXPECT_TRUE(IsAllowedPref("brave.de_amp.enabled"));
  EXPECT_FALSE(IsAllowedPref("no.such.preference"));
  EXPECT_FALSE(IsAllowedPref(""));
  // Paths are matched exactly, not by prefix, so a partial path can't be used
  // to reach a preference that wasn't allowlisted.
  EXPECT_FALSE(IsAllowedPref("brave.de_amp"));
  EXPECT_FALSE(IsAllowedPref("brave"));
  // Preferences that can hold free-form user content must stay out.
  EXPECT_FALSE(IsAllowedPref("download.default_directory"));
  EXPECT_FALSE(IsAllowedPref("session.startup_urls"));
  EXPECT_FALSE(IsAllowedPref("homepage"));
  EXPECT_FALSE(IsAllowedPref("proxy"));
  // brave.ad_default is registered but read by nothing; the real Shields ad
  // blocking default lives in HostContentSettingsMap.
  EXPECT_FALSE(IsAllowedPref("brave.ad_default"));
}

TEST_F(BrowserSettingsRegistryTest, SearchFindsPrefByNaturalLanguage) {
  EXPECT_THAT(SearchIds("clear my cookies when I close the browser"),
              testing::Contains("browser.clear_data.cookies_on_exit"));
  EXPECT_THAT(SearchIds("google amp redirect"),
              testing::Contains("brave.de_amp.enabled"));
  EXPECT_THAT(SearchIds("is safe browsing on"),
              testing::Contains("safebrowsing.enabled"));
  EXPECT_THAT(SearchIds("vertical tabs"),
              testing::Contains("brave.tabs.vertical_tabs_enabled"));
  EXPECT_THAT(SearchIds("wallet auto lock"),
              testing::Contains("brave.wallet.auto_lock_minutes"));
}

// Without descriptions or aliases the only signal is the path itself, so
// searches have to use the vocabulary Brave uses internally. These cases
// document that limitation rather than asserting ideal behaviour.
TEST_F(BrowserSettingsRegistryTest, SearchMatchesOnPathVocabularyOnly) {
  // "ai chat" is the internal name and is found.
  EXPECT_THAT(SearchIds("ai chat memory"),
              testing::Contains("brave.ai_chat.user_memory_enabled"));
  // "leo" is the product name and appears in no path, so it finds nothing.
  EXPECT_THAT(
      SearchIds("leo"),
      testing::Not(testing::Contains("brave.ai_chat.user_memory_enabled")));
  // Brave News is stored under the legacy "today" namespace.
  EXPECT_THAT(SearchIds("today toolbar button"),
              testing::Contains("brave.today.should_show_toolbar_button"));
}

TEST_F(BrowserSettingsRegistryTest, SearchByExactPathReturnsOnlyThatPref) {
  auto matches = SearchPrefs("brave.tabs.hover_mode", 10);
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0].pref_name, "brave.tabs.hover_mode");
}

TEST_F(BrowserSettingsRegistryTest, SearchRespectsMaxResults) {
  EXPECT_LE(SearchPrefs("clear data on exit", 3).size(), 3u);
  EXPECT_TRUE(SearchPrefs("clear data on exit", 0).empty());
}

TEST_F(BrowserSettingsRegistryTest, SearchIsOrderedByDescendingScore) {
  auto matches = SearchPrefs("vertical tabs", 40);
  ASSERT_GE(matches.size(), 2u);
  for (size_t i = 1; i < matches.size(); ++i) {
    EXPECT_GE(matches[i - 1].score, matches[i].score);
  }
}

// Without stop word filtering the filler in a natural question would match
// almost every path -- especially "brave", which prefixes over a hundred of
// them.
TEST_F(BrowserSettingsRegistryTest, SearchIgnoresStopWords) {
  EXPECT_TRUE(SearchPrefs("what is my brave browser setting", 40).empty());
  EXPECT_TRUE(SearchPrefs("brave", 40).empty());
  EXPECT_TRUE(SearchPrefs("", 10).empty());
  EXPECT_TRUE(SearchPrefs("   ", 10).empty());
}

TEST_F(BrowserSettingsRegistryTest, SearchReturnsNothingForUnrelatedQuery) {
  EXPECT_TRUE(SearchPrefs("zzzzqqqq nonexistent gibberish", 10).empty());
}

}  // namespace ai_chat::browser_settings
