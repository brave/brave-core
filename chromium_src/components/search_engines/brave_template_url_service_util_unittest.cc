/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stddef.h>

#include <memory>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/util.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kCountryIDAtInstall[] = "countryid_at_install";

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
  void SetUp() override {
    TemplateURLPrepopulateData::RegisterProfilePrefs(prefs_.registry());
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable prefs_;
};

TEST_F(BraveTemplateURLServiceUtilTest, GetSearchProvidersUsingKeywordResult) {
  std::vector<TemplateURLData> local_turls;
  // Create a sets of TURLs in order different from prepopulated TURLs.
  local_turls.push_back(*CreatePrepopulateTemplateURLData(511, ":sp", 1));
  local_turls.push_back(*CreatePrepopulateTemplateURLData(15, ":ya", 2));
  local_turls.push_back(*CreatePrepopulateTemplateURLData(3, ":b", 3));
  local_turls.push_back(*CreatePrepopulateTemplateURLData(507, ":q", 4));
  local_turls.push_back(*CreatePrepopulateTemplateURLData(501, ":d", 5));
  local_turls.push_back(*CreatePrepopulateTemplateURLData(1, ":g", 6));
  std::unique_ptr<TemplateURL> default_turl =
      std::make_unique<TemplateURL>(local_turls.back());

  // Add TURLs with PrepopulateID that doesn't exist in prepopulated TURLs.
  local_turls.push_back(*CreatePrepopulateTemplateURLData(0, "random1", 7));
  local_turls.push_back(*CreatePrepopulateTemplateURLData(1004, "random2", 8));

  // Prepare call arguments
  WDKeywordsResult kwResult;
  kwResult.builtin_keyword_version =
      TemplateURLPrepopulateData::GetDataVersion(&prefs_);
  kwResult.default_search_provider_id = 2;
  kwResult.keywords = local_turls;
  WDResult<WDKeywordsResult> result(KEYWORDS_RESULT, kwResult);

  TemplateURL::OwnedTemplateURLVector template_urls;
  int new_resource_keyword_version = 0;

  GetSearchProvidersUsingKeywordResult(result, nullptr, &prefs_, &template_urls,
                                       default_turl.get(), SearchTermsData(),
                                       &new_resource_keyword_version, nullptr);

  // Verify count.
  EXPECT_EQ(local_turls.size(), template_urls.size());
  // Verify order.
  EXPECT_EQ(template_urls[0]->keyword(), base::ASCIIToUTF16(":g"));
  EXPECT_EQ(template_urls[1]->keyword(), base::ASCIIToUTF16(":d"));
  EXPECT_EQ(template_urls[2]->keyword(), base::ASCIIToUTF16(":q"));
  EXPECT_EQ(template_urls[3]->keyword(), base::ASCIIToUTF16(":b"));
  EXPECT_EQ(template_urls[4]->keyword(), base::ASCIIToUTF16(":sp"));
  EXPECT_EQ(template_urls[5]->keyword(), base::ASCIIToUTF16(":ya"));
  EXPECT_EQ(template_urls[6]->keyword(), base::ASCIIToUTF16("random1"));
  EXPECT_EQ(template_urls[7]->keyword(), base::ASCIIToUTF16("random2"));

  // Reset
  template_urls.clear();
  new_resource_keyword_version = 0;

  // Check Germany.
  prefs_.SetInteger(kCountryIDAtInstall, 'D' << 8 | 'E');

  GetSearchProvidersUsingKeywordResult(result, nullptr, &prefs_, &template_urls,
                                       default_turl.get(), SearchTermsData(),
                                       &new_resource_keyword_version, nullptr);

  // Verify count.
  EXPECT_EQ(local_turls.size(), template_urls.size());
  // Verify order.
  EXPECT_EQ(template_urls[0]->keyword(), base::ASCIIToUTF16(":q"));
  EXPECT_EQ(template_urls[1]->keyword(), base::ASCIIToUTF16(":g"));
  EXPECT_EQ(template_urls[2]->keyword(), base::ASCIIToUTF16(":d"));
  EXPECT_EQ(template_urls[3]->keyword(), base::ASCIIToUTF16(":b"));
  EXPECT_EQ(template_urls[4]->keyword(), base::ASCIIToUTF16(":sp"));
  EXPECT_EQ(template_urls[5]->keyword(), base::ASCIIToUTF16(":ya"));
  EXPECT_EQ(template_urls[6]->keyword(), base::ASCIIToUTF16("random1"));
  EXPECT_EQ(template_urls[7]->keyword(), base::ASCIIToUTF16("random2"));
}
