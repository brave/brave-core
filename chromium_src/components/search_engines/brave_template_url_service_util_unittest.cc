/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stddef.h>

#include <memory>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/country_codes/country_codes.h"
#include "components/search_engines/search_engine_choice/search_engine_choice_service.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/util.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace TemplateURLPrepopulateData {
extern void LocalizeEngineList(int country_id,
                               std::vector<BravePrepopulatedEngineID>* engines);
}

namespace {

constexpr char kCountryIDAtInstall[] = "countryid_at_install";

std::unique_ptr<TemplateURLData> CreatePrepopulateTemplateURLData(
    int prepopulate_id,
    const std::string& keyword,
    TemplateURLID id) {
  std::unique_ptr<TemplateURLData> data(new TemplateURLData);
  data->prepopulate_id = prepopulate_id;
  data->SetKeyword(base::ASCIIToUTF16(keyword));
  data->id = id;
  return data;
}

}  // namespace

class BraveTemplateURLServiceUtilTest : public testing::Test {
 public:
  BraveTemplateURLServiceUtilTest()
      : search_engine_choice_service_(
            prefs_,
            &local_state_,
            /*is_profile_eligible_for_dse_guest_propagation=*/false) {}
  void SetUp() override {
    TemplateURLPrepopulateData::RegisterProfilePrefs(prefs_.registry());
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable prefs_;
  TestingPrefServiceSimple local_state_;
  search_engines::SearchEngineChoiceService search_engine_choice_service_;
};

void TestDefaultOrder(const TemplateURL::OwnedTemplateURLVector& template_urls,
                      const std::vector<std::string>& keywords) {
  EXPECT_EQ(template_urls.size(), keywords.size());

  for (size_t i = 0; i < template_urls.size(); i++) {
    EXPECT_EQ(template_urls[i]->keyword(), base::ASCIIToUTF16(keywords[i]));
  }
}

std::vector<TemplateURLData> GetSampleTemplateData() {
  std::vector<TemplateURLData> local_turls;

  // Create a sets of TURLs in order different from prepopulated TURLs.
  local_turls.push_back(*CreatePrepopulateTemplateURLData(
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_STARTPAGE, ":sp", 1));
  local_turls.push_back(*CreatePrepopulateTemplateURLData(15, ":ya", 2));
  local_turls.push_back(*CreatePrepopulateTemplateURLData(
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING, ":b", 3));
  local_turls.push_back(*CreatePrepopulateTemplateURLData(
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT, ":q", 4));
  local_turls.push_back(*CreatePrepopulateTemplateURLData(
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO, ":d", 5));
  local_turls.push_back(*CreatePrepopulateTemplateURLData(
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE, ":g", 6));

  return local_turls;
}

WDKeywordsResult InitKeywordResult(
    sync_preferences::TestingPrefServiceSyncable* prefs,
    const std::vector<TemplateURLData>& local_turls) {
  WDKeywordsResult kwResult;
  kwResult.metadata.builtin_keyword_data_version =
      TemplateURLPrepopulateData::GetDataVersion(prefs);
  kwResult.keywords = local_turls;
  return kwResult;
}

TEST_F(BraveTemplateURLServiceUtilTest, GetSearchProvidersUsingKeywordResult) {
  std::vector<TemplateURLData> local_turls = GetSampleTemplateData();
  std::unique_ptr<TemplateURL> default_turl =
      std::make_unique<TemplateURL>(local_turls.back());

  // Add TURLs with PrepopulateID that doesn't exist in prepopulated TURLs.
  local_turls.push_back(*CreatePrepopulateTemplateURLData(0, "random1", 7));
  local_turls.push_back(*CreatePrepopulateTemplateURLData(1004, "random2", 8));

  // Prepare call arguments
  WDResult<WDKeywordsResult> result(KEYWORDS_RESULT,
                                    InitKeywordResult(&prefs_, local_turls));

  TemplateURL::OwnedTemplateURLVector template_urls;
  WDKeywordsResult::Metadata updated_keywords_metadata;

  prefs_.SetInteger(kCountryIDAtInstall, 'U' << 8 | 'S');
  GetSearchProvidersUsingKeywordResult(
      result, nullptr, &prefs_, &search_engine_choice_service_, &template_urls,
      default_turl.get(), SearchTermsData(), updated_keywords_metadata,
      nullptr);

  // Verify count and order.
  TestDefaultOrder(template_urls,
                   {":g", ":d", ":q", ":b", ":sp", ":ya", "random1", "random2",
                    "@bookmarks", "@history", "@tabs", "@gemini"});
}

TEST_F(BraveTemplateURLServiceUtilTest,
       GetSearchProvidersUsingKeywordResultGermany) {
  std::vector<TemplateURLData> local_turls = GetSampleTemplateData();
  std::unique_ptr<TemplateURL> default_turl =
      std::make_unique<TemplateURL>(local_turls.back());

  // Germany specific query param
  local_turls[4].prepopulate_id =
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE;

  // Prepare call arguments
  WDResult<WDKeywordsResult> result(KEYWORDS_RESULT,
                                    InitKeywordResult(&prefs_, local_turls));
  TemplateURL::OwnedTemplateURLVector template_urls;
  WDKeywordsResult::Metadata updated_keywords_metadata;

  // Check Germany.
  prefs_.SetInteger(kCountryIDAtInstall, 'D' << 8 | 'E');
  GetSearchProvidersUsingKeywordResult(
      result, nullptr, &prefs_, &search_engine_choice_service_, &template_urls,
      default_turl.get(), SearchTermsData(), updated_keywords_metadata,
      nullptr);

  // Verify count and order.
  TestDefaultOrder(template_urls,
                   {":br", ":d", ":q", ":g", ":b", ":sp", ":e", ":ya",
                    "@bookmarks", "@history", "@tabs", "@gemini"});
}
