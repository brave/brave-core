/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BravePrepopulatedEnginesTest, ModifiedProviderTest) {
  auto data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      nullptr, nullptr,
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING);
  // Check modified bing provider url.
  EXPECT_EQ(data->url(), "https://www.bing.com/search?q={searchTerms}");
}
