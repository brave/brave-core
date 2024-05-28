/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include <stddef.h>

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "base/command_line.h"
#include "base/files/scoped_temp_dir.h"
#include "base/stl_util.h"
#include "base/values.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/google/core/common/google_switches.h"
#include "components/prefs/testing_pref_service.h"
#include "components/search_engines/search_engine_choice/search_engine_choice_service.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/testing_search_terms_data.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kCountryIDAtInstall[] = "countryid_at_install";

std::string GetHostFromTemplateURLData(const TemplateURLData& data) {
  return TemplateURL(data).url_ref().GetHost(SearchTermsData());
}

using namespace TemplateURLPrepopulateData;  // NOLINT

const PrepopulatedEngine* const kBraveAddedEngines[] = {
    &startpage,
};

const std::unordered_set<std::u16string_view> kOverriddenEnginesNames = {
    u"DuckDuckGo", u"Qwant"};

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
  BraveTemplateURLPrepopulateDataTest()
      : search_engine_choice_service_(prefs_, &local_state_) {}
  void SetUp() override {
    TemplateURLPrepopulateData::RegisterProfilePrefs(prefs_.registry());
    // Real registration happens in `brave/browser/brave_profile_prefs.cc`
    // Calling brave::RegisterProfilePrefs() causes some problems though
    auto* registry = prefs_.registry();
    registry->RegisterIntegerPref(
        prefs::kBraveDefaultSearchVersion,
        TemplateURLPrepopulateData::kBraveCurrentDataVersion);
  }

  void CheckForCountry(char digit1, char digit2, int prepopulate_id) {
    prefs_.SetInteger(kCountryIDAtInstall, digit1 << 8 | digit2);
    prefs_.SetInteger(prefs::kBraveDefaultSearchVersion,
                      TemplateURLPrepopulateData::kBraveCurrentDataVersion);
    size_t default_index;
    std::vector<std::unique_ptr<TemplateURLData>> t_urls =
        TemplateURLPrepopulateData::GetPrepopulatedEngines(
            &prefs_, &search_engine_choice_service_, &default_index);
    EXPECT_EQ(prepopulate_id, t_urls[default_index]->prepopulate_id);
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable prefs_;
  TestingPrefServiceSimple local_state_;
  search_engines::SearchEngineChoiceService search_engine_choice_service_;
};

// Verifies that the set of all prepopulate data doesn't contain entries with
// duplicate keywords. This should make us notice if Chromium adds a search
// engine in the future that Brave already added.
TEST_F(BraveTemplateURLPrepopulateDataTest, UniqueKeywords) {
  using PrepopulatedEngine = TemplateURLPrepopulateData::PrepopulatedEngine;
  const std::vector<const PrepopulatedEngine*> all_engines =
      ::GetAllPrepopulatedEngines();
  std::set<std::u16string_view> unique_keywords;
  for (const PrepopulatedEngine* engine : all_engines) {
    ASSERT_TRUE(unique_keywords.find(engine->keyword) == unique_keywords.end());
    unique_keywords.insert(engine->keyword);
  }
}

// Verifies that engines we override are used and not the original engines.
TEST_F(BraveTemplateURLPrepopulateDataTest, OverriddenEngines) {
  using PrepopulatedEngine = TemplateURLPrepopulateData::PrepopulatedEngine;
  const std::vector<const PrepopulatedEngine*> all_engines =
      ::GetAllPrepopulatedEngines();
  for (const PrepopulatedEngine* engine : all_engines) {
    if (kOverriddenEnginesNames.count(engine->name) > 0)
      ASSERT_GE(static_cast<unsigned int>(engine->id),
                TemplateURLPrepopulateData::BRAVE_PREPOPULATED_ENGINES_START);
  }
}

// Verifies that the set of prepopulate data for each locale
// doesn't contain entries with duplicate ids.
TEST_F(BraveTemplateURLPrepopulateDataTest, UniqueIDs) {
  const int kCountryIds[] = {'D' << 8 | 'E', 'F' << 8 | 'R', 'U' << 8 | 'S',
                             -1};

  for (int country_id : kCountryIds) {
    prefs_.SetInteger(kCountryIDAtInstall, country_id);
    std::vector<std::unique_ptr<TemplateURLData>> urls = GetPrepopulatedEngines(
        &prefs_, &search_engine_choice_service_, nullptr);
    std::set<int> unique_ids;
    for (auto& url : urls) {
      ASSERT_TRUE(unique_ids.find(url->prepopulate_id) == unique_ids.end());
      unique_ids.insert(url->prepopulate_id);
    }
  }
}

// Verifies that each prepopulate data entry has required fields
TEST_F(BraveTemplateURLPrepopulateDataTest, ProvidersFromPrepopulated) {
  size_t default_index;
  std::vector<std::unique_ptr<TemplateURLData>> t_urls =
      TemplateURLPrepopulateData::GetPrepopulatedEngines(
          &prefs_, &search_engine_choice_service_, &default_index);

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
TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForArgentina) {
  CheckForCountry('A', 'R', PREPOPULATED_ENGINE_ID_BRAVE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest, DefaultSearchProvidersForIndia) {
  CheckForCountry('I', 'N', PREPOPULATED_ENGINE_ID_BRAVE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest, DefaultSearchProvidersForBrazil) {
  CheckForCountry('B', 'R', PREPOPULATED_ENGINE_ID_BRAVE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest, DefaultSearchProvidersForUSA) {
  CheckForCountry('U', 'S', PREPOPULATED_ENGINE_ID_BRAVE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest, DefaultSearchProvidersForGermany) {
  CheckForCountry('D', 'E', PREPOPULATED_ENGINE_ID_BRAVE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest, DefaultSearchProvidersForFrance) {
  CheckForCountry('F', 'R', PREPOPULATED_ENGINE_ID_BRAVE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForGreatBritain) {
  CheckForCountry('G', 'B', PREPOPULATED_ENGINE_ID_BRAVE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest, DefaultSearchProvidersForCanada) {
  CheckForCountry('C', 'A', PREPOPULATED_ENGINE_ID_BRAVE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForAustralia) {
  CheckForCountry('A', 'U', PREPOPULATED_ENGINE_ID_GOOGLE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForNewZealand) {
  CheckForCountry('N', 'Z', PREPOPULATED_ENGINE_ID_GOOGLE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest, DefaultSearchProvidersForIreland) {
  CheckForCountry('I', 'E', PREPOPULATED_ENGINE_ID_GOOGLE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest, DefaultSearchProvidersForAustria) {
  CheckForCountry('A', 'T', PREPOPULATED_ENGINE_ID_BRAVE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest, DefaultSearchProvidersForSpain) {
  CheckForCountry('E', 'S', PREPOPULATED_ENGINE_ID_BRAVE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest, DefaultSearchProvidersForMexico) {
  CheckForCountry('M', 'X', PREPOPULATED_ENGINE_ID_BRAVE);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForRepublicOfArmenia) {
  CheckForCountry('A', 'M', PREPOPULATED_ENGINE_ID_YANDEX);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForRepublicOfAzerbaijan) {
  CheckForCountry('A', 'Z', PREPOPULATED_ENGINE_ID_YANDEX);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForRepublicOfBelarus) {
  CheckForCountry('B', 'Y', PREPOPULATED_ENGINE_ID_YANDEX);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForKyrgyzRepublic) {
  CheckForCountry('K', 'G', PREPOPULATED_ENGINE_ID_YANDEX);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForRepublicOfKazakhstan) {
  CheckForCountry('K', 'Z', PREPOPULATED_ENGINE_ID_YANDEX);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForRepublicOfMoldova) {
  CheckForCountry('M', 'D', PREPOPULATED_ENGINE_ID_YANDEX);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForRussianFederation) {
  CheckForCountry('R', 'U', PREPOPULATED_ENGINE_ID_YANDEX);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForRepublicOfTajikistan) {
  CheckForCountry('T', 'J', PREPOPULATED_ENGINE_ID_YANDEX);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForTurkmenistan) {
  CheckForCountry('T', 'M', PREPOPULATED_ENGINE_ID_YANDEX);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForRepublicOfUzbekistan) {
  CheckForCountry('U', 'Z', PREPOPULATED_ENGINE_ID_YANDEX);
}

TEST_F(BraveTemplateURLPrepopulateDataTest,
       DefaultSearchProvidersForSouthKorea) {
  CheckForCountry('K', 'R', PREPOPULATED_ENGINE_ID_NAVER);
}

TEST_F(BraveTemplateURLPrepopulateDataTest, DefaultSearchProvidersForItaly) {
  CheckForCountry('I', 'T', PREPOPULATED_ENGINE_ID_BRAVE);
}
