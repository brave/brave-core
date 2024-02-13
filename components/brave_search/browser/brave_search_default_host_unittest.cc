// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/browser/brave_search_default_host.h"

#include <map>
#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/brave_search/browser/prefs.h"
#include "brave/components/brave_search/common/features.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_search {
namespace {

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

std::unique_ptr<TemplateURL> CreateSearchProvider(std::string host) {
  std::string base_url = "https://" + host + '/';
  TemplateURLData data;
  data.SetShortName(base::UTF8ToUTF16(base_url));
  data.SetKeyword(base::UTF8ToUTF16(base_url));
  data.SetURL(base_url + "url?bar={searchTerms}");
  data.new_tab_url = base_url + "newtab";
  data.alternate_urls.push_back(base_url + "alt#quux={searchTerms}");
  return std::make_unique<TemplateURL>(data);
}

class BraveSearchDefaultHostTest : public ::testing::Test {
 protected:
  using MockGetCanSetCallback = base::MockCallback<
      BraveSearchDefaultHost::GetCanSetDefaultSearchProviderCallback>;

  BraveSearchDefaultHostTest()
      : template_url_service_(
            static_cast<TemplateURLService::Initializer*>(nullptr),
            0u) {
    feature_list_.InitAndEnableFeatureWithParameters(
        brave_search::features::kBraveSearchDefaultAPIFeature,
        {{brave_search::features::kBraveSearchDefaultAPIDailyLimitName, "3"},
         {brave_search::features::kBraveSearchDefaultAPITotalLimitName, "10"}});
  }

  ~BraveSearchDefaultHostTest() override = default;

  void SetUp() override {
    pref_service_.registry()->RegisterListPref(prefs::kDailyAsked);
    pref_service_.registry()->RegisterIntegerPref(prefs::kTotalAsked, 0);
    brave_search_conversion::RegisterPrefs(pref_service_.registry());
    PrepareFieldTrialParamsForBannerTypeC();
  }

  void TearDown() override {
    pref_service_.ClearPref(prefs::kDailyAsked);
    pref_service_.ClearPref(prefs::kTotalAsked);
  }

  void PrepareFieldTrialParamsForBannerTypeC() {
    constexpr char kPromotionTrial[] = "BraveSearchPromotionBannerStudy";
    constexpr char kBannerTypeParamName[] = "banner_type";
    constexpr char kBannerTypeExperiements[] = "banner_type_c";

    std::map<std::string, std::string> params;
    params[kBannerTypeParamName] = "type_C";
    ASSERT_TRUE(base::AssociateFieldTrialParams(
        kPromotionTrial, kBannerTypeExperiements, params));
    base::FieldTrialList::CreateFieldTrial(kPromotionTrial,
                                           kBannerTypeExperiements);
  }

  std::unique_ptr<BraveSearchDefaultHost> GetAPIHost(const std::string& host) {
    return std::make_unique<BraveSearchDefaultHost>(
        host, &template_url_service_, &pref_service_);
  }

  TemplateURL* AddSearchProviderForHost(const std::string& host) {
    return template_url_service_.Add(CreateSearchProvider(host));
  }

  TemplateURLService template_url_service_;
  TestingPrefServiceSimple pref_service_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveSearchDefaultHostTest, AllowsIfPresent) {
  auto host = GetAPIHost("search.test.com");
  // Add a search provider
  AddSearchProviderForHost("search.test.com");
  // Verify can ask to set default
  MockGetCanSetCallback first;
  EXPECT_CALL(first, Run(true));
  host->GetCanSetDefaultSearchProvider(first.Get());
}

TEST_F(BraveSearchDefaultHostTest, DisallowsIfNotPresent) {
  auto host = GetAPIHost("search.test.com");
  // Do not add search provider
  // Verify can not ask to set default
  MockGetCanSetCallback first;
  EXPECT_CALL(first, Run(false));
  host->GetCanSetDefaultSearchProvider(first.Get());
}

TEST_F(BraveSearchDefaultHostTest, DisallowsIfDefault) {
  auto host = GetAPIHost("search.test.com");
  // Add a search provider for the host
  auto* provider = AddSearchProviderForHost("search.test.com");
  ASSERT_TRUE(template_url_service_.CanMakeDefault(provider));
  template_url_service_.SetUserSelectedDefaultSearchProvider(provider);
  // Verify can not ask to set default
  MockGetCanSetCallback first;
  EXPECT_CALL(first, Run(false));
  host->GetCanSetDefaultSearchProvider(first.Get());
}

TEST_F(BraveSearchDefaultHostTest, AllowsIfNotDefault) {
  auto host = GetAPIHost("search.test.com");
  // Add a search provider for the host
  AddSearchProviderForHost("search.test.com");
  // Make another search provider default
  auto* default_provider = AddSearchProviderForHost("search.test2.com");
  ASSERT_TRUE(template_url_service_.CanMakeDefault(default_provider));
  template_url_service_.SetUserSelectedDefaultSearchProvider(default_provider);
  // Verify can set default since current host is not default
  MockGetCanSetCallback first;
  EXPECT_CALL(first, Run(true));
  host->GetCanSetDefaultSearchProvider(first.Get());
}

TEST_F(BraveSearchDefaultHostTest, DisallowsAfterMaxTimesAsked) {
  auto host = GetAPIHost("search.test.com");
  // Add a search provider for the host
  AddSearchProviderForHost("search.test.com");
  // Verify can initially set default
  MockGetCanSetCallback first, second, third, fourth;
  EXPECT_CALL(first, Run(true));
  EXPECT_CALL(second, Run(true));
  EXPECT_CALL(third, Run(true));
  EXPECT_CALL(fourth, Run(false));
  // Callback is called synchronously, so we don't need to wait.
  host->GetCanSetDefaultSearchProvider(first.Get());
  host->GetCanSetDefaultSearchProvider(second.Get());
  host->GetCanSetDefaultSearchProvider(third.Get());
  host->GetCanSetDefaultSearchProvider(fourth.Get());
}

TEST_F(BraveSearchDefaultHostTest, CanSetDefaultAlwaysTestWithSearchPromotion) {
  base::test::ScopedFeatureList feature_list;

  brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_US"};

  auto host = GetAPIHost("search.test.com");
  // Add a search provider for the host
  AddSearchProviderForHost("search.test.com");
  // Make another search provider default
  auto* default_provider = AddSearchProviderForHost("search.test2.com");
  ASSERT_TRUE(template_url_service_.CanMakeDefault(default_provider));
  template_url_service_.SetUserSelectedDefaultSearchProvider(default_provider);

  // Failed at fourth try by default.
  MockGetCanSetCallback first, second, third, fourth, fifth, sixth, seventh,
      eighth;
  EXPECT_CALL(first, Run(true));
  EXPECT_CALL(second, Run(true));
  EXPECT_CALL(third, Run(true));
  EXPECT_CALL(fourth, Run(false));

  host->GetCanSetDefaultSearchProvider(first.Get());
  host->GetCanSetDefaultSearchProvider(second.Get());
  host->GetCanSetDefaultSearchProvider(third.Get());
  host->GetCanSetDefaultSearchProvider(fourth.Get());

  feature_list.InitAndEnableFeature(
      brave_search_conversion::features::kOmniboxBanner);
  host->SetCanAlwaysSetDefault();

  // Can set after calling SetCanAlwaysSetDefault() with omnibox banner
  // promotion features enabled.
  EXPECT_CALL(fifth, Run(true));
  host->GetCanSetDefaultSearchProvider(fifth.Get());

  feature_list.Reset();
  host->SetCanAlwaysSetDefault();

  // Can't set if promotion feature is disabled.
  EXPECT_CALL(sixth, Run(false));
  host->GetCanSetDefaultSearchProvider(sixth.Get());

  // Test with omnibox button type conversion.
  feature_list.InitAndEnableFeature(
      brave_search_conversion::features::kOmniboxButton);
  host->SetCanAlwaysSetDefault();

  // Can set if omnibox button promotion feature is enabled.
  EXPECT_CALL(seventh, Run(true));
  host->GetCanSetDefaultSearchProvider(seventh.Get());

  feature_list.Reset();
  feature_list.InitAndEnableFeature(brave_search_conversion::features::kNTP);
  host->SetCanAlwaysSetDefault();

  // Can set if ntp promotion feature is enabled.
  EXPECT_CALL(eighth, Run(true));
  host->GetCanSetDefaultSearchProvider(eighth.Get());
}

}  // namespace
}  // namespace brave_search
