// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/search_engines/template_url_prepopulate_data.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/google/core/common/google_switches.h"
#include "components/search_engines/prepopulated_engines.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/testing_search_terms_data.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;

namespace {

std::string GetHostFromTemplateURLData(const TemplateURLData& data) {
  return TemplateURL(data).url_ref().GetHost(SearchTermsData());
}

using namespace TemplateURLPrepopulateData;
const PrepopulatedEngine* const kBraveAddedEngines[] = {
    &amazon,          &duckduckgo,    &ecosia,    &findx,
    &github,          &mdnwebdocs,    &qwant,     &searx,
    &semanticscholar, &stackoverflow, &startpage, &twitter,
    &wikipedia,       &wolframalpha,  &yandex,    &youtube,
};

std::vector<const PrepopulatedEngine*> GetAllPrepopulatedEngines() {
  std::vector<const PrepopulatedEngine*> engines =
      TemplateURLPrepopulateData::GetAllPrepopulatedEngines();
  engines.insert(engines.end(), std::begin(kBraveAddedEngines),
                 std::end(kBraveAddedEngines));
  return engines;
}

}  // namespace

class BraveTemplateURLPrepopulateDataTest : public testing::Test {
 public:
  void SetUp() override {
    TemplateURLPrepopulateData::RegisterProfilePrefs(prefs_.registry());
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable prefs_;
};

// Verifies that the set of all prepopulate data doesn't contain entries with
// duplicate keywords. This should make us notice if Chromium adds a search
// engine in the future that Brave already added.
TEST_F(BraveTemplateURLPrepopulateDataTest, UniqueKeywords) {
  using PrepopulatedEngine = TemplateURLPrepopulateData::PrepopulatedEngine;
  const std::vector<const PrepopulatedEngine*> all_engines =
      ::GetAllPrepopulatedEngines();
  std::set<std::wstring> unique_keywords;
  for (const PrepopulatedEngine* engine : all_engines) {
    ASSERT_TRUE(unique_keywords.find(engine->keyword) == unique_keywords.end());
    unique_keywords.insert(engine->keyword);
  }
}

// Verifies that the set of prepopulate data for each locale
// doesn't contain entries with duplicate ids.
TEST_F(BraveTemplateURLPrepopulateDataTest, UniqueIDs) {
  const int kCountryIds[] = {
    'C' << 8 | 'A',
    'D' << 8 | 'E',
    'F' << 8 | 'R',
    'U' << 8 | 'S',
    -1
  };

  for (size_t i = 0; i < arraysize(kCountryIds); ++i) {
    prefs_.SetInteger(prefs::kCountryIDAtInstall, kCountryIds[i]);
    std::vector<std::unique_ptr<TemplateURLData>> urls =
        GetPrepopulatedEngines(&prefs_, nullptr);
    std::set<int> unique_ids;
    for (size_t turl_i = 0; turl_i < urls.size(); ++turl_i) {
      ASSERT_TRUE(unique_ids.find(urls[turl_i]->prepopulate_id) ==
                  unique_ids.end());
      unique_ids.insert(urls[turl_i]->prepopulate_id);
    }
  }
}

// Verifies that each prepopulate data entry has required fields
TEST_F(BraveTemplateURLPrepopulateDataTest, ProvidersFromPrepopulated) {
  size_t default_index;
  std::vector<std::unique_ptr<TemplateURLData>> t_urls =
      TemplateURLPrepopulateData::GetPrepopulatedEngines(&prefs_,
                                                         &default_index);

  // Ensure all the URLs have the required fields populated.
  ASSERT_FALSE(t_urls.empty());
  for (size_t i = 0; i < t_urls.size(); ++i) {
    ASSERT_FALSE(t_urls[i]->short_name().empty());
    ASSERT_FALSE(t_urls[i]->keyword().empty());
    ASSERT_FALSE(t_urls[i]->favicon_url.host().empty());
    ASSERT_FALSE(GetHostFromTemplateURLData(*t_urls[i]).empty());
    ASSERT_FALSE(t_urls[i]->input_encodings.empty());
    EXPECT_GT(t_urls[i]->prepopulate_id, 0);
    EXPECT_TRUE(t_urls[0]->safe_for_autoreplace);
    EXPECT_TRUE(t_urls[0]->date_created.is_null());
    EXPECT_TRUE(t_urls[0]->last_modified.is_null());
  }
}

// Verifies default search provider for locale
TEST_F(BraveTemplateURLPrepopulateDataTest, DefaultSearchProvidersForLocale) {
  // Use United States.
  prefs_.SetInteger(prefs::kCountryIDAtInstall, 'U' << 8 | 'S');
  size_t default_index;
  std::vector<std::unique_ptr<TemplateURLData>> t_urls =
      TemplateURLPrepopulateData::GetPrepopulatedEngines(&prefs_,
                                                         &default_index);
  EXPECT_EQ(ASCIIToUTF16("Google"), t_urls[default_index]->short_name());

  t_urls.clear();

  // Use Germany.
  prefs_.SetInteger(prefs::kCountryIDAtInstall, 'D' << 8 | 'E');
  t_urls = TemplateURLPrepopulateData::GetPrepopulatedEngines(&prefs_,
                                                              &default_index);
  EXPECT_EQ(ASCIIToUTF16("Qwant"), t_urls[default_index]->short_name());

  t_urls.clear();

  // Use France.
  prefs_.SetInteger(prefs::kCountryIDAtInstall, 'F' << 8 | 'R');
  t_urls = TemplateURLPrepopulateData::GetPrepopulatedEngines(&prefs_,
                                                              &default_index);
  EXPECT_EQ(ASCIIToUTF16("Qwant"), t_urls[default_index]->short_name());
}
