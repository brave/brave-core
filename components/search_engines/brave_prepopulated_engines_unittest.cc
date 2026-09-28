/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/search_engines/brave_prepopulated_engines.h"

#include <cstddef>
#include <string>

#include "base/test/task_environment.h"
#include "build/build_config.h"
#include "components/country_codes/country_codes.h"
#include "components/regional_capabilities/regional_capabilities_prefs.h"
#include "components/regional_capabilities/regional_capabilities_service.h"
#include "components/search_engines/search_engine_choice/search_engine_choice_service.h"
#include "components/search_engines/search_engines_test_environment.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "net/base/url_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class BravePrepopulatedEnginesTest : public testing::Test {
 public:
  BravePrepopulatedEnginesTest() = default;

 protected:
  base::test::SingleThreadTaskEnvironment task_environment_;
  search_engines::SearchEnginesTestEnvironment search_engines_test_environment_;
};

TEST_F(BravePrepopulatedEnginesTest, ModifiedProviderTest) {
  auto data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      search_engines_test_environment_.pref_service(),
      search_engines_test_environment_.regional_capabilities_service()
          .GetRegionalPrepopulatedEngines(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING);
  // Check modified bing provider url.
  EXPECT_EQ(data->url(), "https://www.bing.com/search?q={searchTerms}");
}

TEST_F(BravePrepopulatedEnginesTest,
       Issue59394YahooJapanSearchUrlDoesNotGetExtraFrParamAppended) {
  search_engines_test_environment_.pref_service().SetInteger(
      regional_capabilities::prefs::kCountryIDAtInstall,
      country_codes::CountryId("JP").Serialize());

  auto data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      search_engines_test_environment_.pref_service(),
      search_engines_test_environment_.regional_capabilities_service()
          .GetRegionalPrepopulatedEngines(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YAHOO_JP);
  ASSERT_TRUE(data);

  TemplateURL template_url(*data);
  GURL search_url(template_url.url_ref().ReplaceSearchTerms(
      TemplateURLRef::SearchTermsArgs(u"query"), SearchTermsData()));

  size_t fr_param_count = 0;
  std::string fr_param_value;
  for (net::QueryIterator it(search_url); !it.IsAtEnd(); it.Advance()) {
    if (it.GetKey() == "fr") {
      ++fr_param_count;
      fr_param_value = it.GetUnescapedValue();
    }
  }
  ASSERT_EQ(1u, fr_param_count) << search_url;
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
  EXPECT_EQ("brave-mobile_ext", fr_param_value);
#else
  EXPECT_EQ("brave-desktop_ext", fr_param_value);
#endif
}

TEST_F(BravePrepopulatedEnginesTest,
       Issue59394YahooJapanHasNoRegulatoryExtensions) {
  EXPECT_THAT(TemplateURLPrepopulateData::brave_yahoo_jp.regulatory_extensions,
              testing::IsEmpty());
}
