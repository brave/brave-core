/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class BravePrepopulatedEnginesTest : public testing::Test {
 public:
  BravePrepopulatedEnginesTest() = default;
  ~BravePrepopulatedEnginesTest() override = default;

  void SetUp() override { profile_ = std::make_unique<TestingProfile>(); }

  content::BrowserTaskEnvironment browser_task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(BravePrepopulatedEnginesTest, ModifiedProviderTest) {
  auto data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      profile_->GetPrefs(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING);
  // Check modified bing provider url.
  EXPECT_EQ(data->url(), "https://www.bing.com/search?q={searchTerms}");
}
