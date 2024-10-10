/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/common/url_readable_hints.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace speedreader {

TEST(SpeedreaderUtilTest, URLHasHints) {
  EXPECT_FALSE(IsURLLooksReadable(GURL("https://github.com/brave/brave-core")));

  // URL has date in path
  EXPECT_TRUE(
      IsURLLooksReadable(GURL("https://www.nytimes.com/2021/05/13/science/"
                              "virus-origins-lab-leak-scientists.html")));

  // URL has "post" in path
  EXPECT_TRUE(IsURLLooksReadable(
      GURL("https://ethan.katzenberg.co.uk/posts/saying-difficult-things/")));

  // URL has "article" in path
  EXPECT_TRUE(IsURLLooksReadable(
      GURL("https://www.nature.com/articles/d41586-021-01332-0")));

  // Has "story" in path
  EXPECT_TRUE(IsURLLooksReadable(
      GURL("https://www.architecturaldigest.com/story/"
           "new-york-city-approved-floating-pool-east-river")));

  // Has "entry" in path
  EXPECT_TRUE(IsURLLooksReadable(
      GURL("https://www.huffpost.com/entry/"
           "asap-rocky-rihanna-relationship_n_60a53b3ce4b09092480b8249")));

  // Ignore case on article
  EXPECT_TRUE(IsURLLooksReadable(GURL("https://lwn.net/Articles/414618/")));

  // Has the blog subdomain
  EXPECT_TRUE(IsURLLooksReadable(
      GURL("https://blog.twitter.com/engineering/en_us/topics/open-source/2021/"
           "dropping-cache-didnt-drop-cache.html")));

  // Has politics in component
  EXPECT_TRUE(IsURLLooksReadable(GURL(
      "https://abcnews.go.com/Politics/"
      "state-dept-ends-policy-denying-us-citizenship-children/"
      "story?id=77743483&cid=clicksource_4380645_5_film_strip_icymi_hed")));

  // 'b' follows "story"
  EXPECT_FALSE(IsURLLooksReadable(GURL("https://fake.com/storyboard")));

  // Has news in component, but is trailing entry
  EXPECT_FALSE(IsURLLooksReadable(
      GURL("https://search.brave.com/news?q=stuff&source=web")));
}
}  // namespace speedreader
