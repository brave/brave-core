// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/browser/brave_search_default_host.h"

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/callback_helpers.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/brave_search/browser/prefs.h"
#include "brave/components/brave_search/common/features.h"
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

  BraveSearchDefaultHostTest() : template_url_service_(nullptr, 0) {
    feature_list_.InitAndEnableFeatureWithParameters(
        brave_search::features::kBraveSearchDefaultAPIFeature,
        {{brave_search::features::kBraveSearchDefaultAPIDailyLimitName, "3"},
         {brave_search::features::kBraveSearchDefaultAPITotalLimitName, "10"}});
  }

  ~BraveSearchDefaultHostTest() override {}

  void SetUp() override {
    pref_service_.registry()->RegisterListPref(prefs::kDailyAsked);
    pref_service_.registry()->RegisterIntegerPref(prefs::kTotalAsked, 0);
  }

  void TearDown() override {
    pref_service_.ClearPref(prefs::kDailyAsked);
    pref_service_.ClearPref(prefs::kTotalAsked);
  }

  BraveSearchDefaultHost* GetAPIHost(const std::string& host) {
    return new BraveSearchDefaultHost(host, &template_url_service_,
                                      &pref_service_);
  }

  TemplateURL* AddSearchProviderForHost(const std::string& host) {
    return template_url_service_.Add(CreateSearchProvider(host));
  }

  TemplateURLService template_url_service_;
  TestingPrefServiceSimple pref_service_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveSearchDefaultHostTest, AllowsIfPresent) {
  BraveSearchDefaultHost* host = GetAPIHost("search.test.com");
  // Add a search provider
  AddSearchProviderForHost("search.test.com");
  // Verify can ask to set default
  MockGetCanSetCallback first;
  EXPECT_CALL(first, Run(true));
  host->GetCanSetDefaultSearchProvider(first.Get());
}

TEST_F(BraveSearchDefaultHostTest, DisallowsIfNotPresent) {
  BraveSearchDefaultHost* host = GetAPIHost("search.test.com");
  // Do not add search provider
  // Verify can not ask to set default
  MockGetCanSetCallback first;
  EXPECT_CALL(first, Run(false));
  host->GetCanSetDefaultSearchProvider(first.Get());
}

TEST_F(BraveSearchDefaultHostTest, DisallowsIfDefault) {
  BraveSearchDefaultHost* host = GetAPIHost("search.test.com");
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
  BraveSearchDefaultHost* host = GetAPIHost("search.test.com");
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
  BraveSearchDefaultHost* host = GetAPIHost("search.test.com");
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

}  // namespace
}  // namespace brave_search
