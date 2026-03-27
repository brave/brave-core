/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/search_engines/template_url.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveTemplateURLTest, GsLcrpOmitted) {
  TemplateURLData data;
  // {google:assistedQueryStats} is the parameter that gets substituted in the
  // upstream with gs_lcrp.
  data.SetURL(
      "https://google.com/?q={searchTerms}&{google:assistedQueryStats}");
  TemplateURL url(data);
  // ReplaceSearchTerms hits the code path that handles replacement of
  // {google:assistedQueryStats}.
  GURL result(url.url_ref().ReplaceSearchTerms({}, {}));
  ASSERT_TRUE(result.is_valid());
  EXPECT_EQ(result.spec(), "https://google.com/?q=&");
}
