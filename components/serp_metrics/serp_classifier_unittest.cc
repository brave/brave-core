/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_classifier.h"

#include <optional>

#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/serp_metrics/serp_classifier_utils.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace serp_metrics {

namespace {

void VerifySerpClassifierExpectation(
    const TemplateURLPrepopulateData::PrepopulatedEngine& prepopulated_engine) {
  const auto template_url_data =
      TemplateURLDataFromPrepopulatedEngine(prepopulated_engine);
  const TemplateURL template_url(*template_url_data);

  const GURL url = template_url.GenerateSearchURL(SearchTermsData(), u"test");
  ASSERT_TRUE(url.is_valid());

  if (std::optional<SearchEngineType> search_engine_type =
          MaybeClassifySearchEngine(url)) {
    EXPECT_TRUE(IsAllowedSearchEngine(*search_engine_type));
  }
}

}  // namespace

TEST(SerpClassifierTest, IsSameSearchQuery) {
  EXPECT_TRUE(IsSameSearchQuery(
      GURL(R"(https://search.brave.com/search?q=foobar)"),
      GURL(R"(https://search.brave.com/search?q=foobar&t=web)")));
}

TEST(SerpClassifierTest, IsSameSearchQueryWithDifferentParamOrder) {
  EXPECT_TRUE(IsSameSearchQuery(
      GURL(R"(https://search.brave.com/search?q=foobar)"),
      GURL(R"(https://search.brave.com/search?t=web&q=foobar)")));
}

TEST(SerpClassifierTest, IsNotSameSearchQuery) {
  EXPECT_FALSE(IsSameSearchQuery(
      GURL(R"(https://search.brave.com/search?q=foo&t=web)"),
      GURL(R"(https://search.brave.com/search?q=bar&t=web")")));
}

TEST(SerpClassifierTest, IsNotSameSearchQueryWithInvalidUrl) {
  EXPECT_FALSE(IsSameSearchQuery(
      GURL(R"(https://search.brave.com/search?q=foobar)"), GURL("invalid")));
}

TEST(SerpClassifierTest, OnlyClassifyAllowedSearchEngines) {
  for (const auto* prepopulated_engine :
       TemplateURLPrepopulateData::GetAllPrepopulatedEngines()) {
    VerifySerpClassifierExpectation(*prepopulated_engine);
  }

  for (const auto& [_, prepopulated_engine] :
       TemplateURLPrepopulateData::kBraveEngines) {
    VerifySerpClassifierExpectation(*prepopulated_engine);
  }
}

TEST(SerpClassifierTest, ClassifyStartpageSearchEngine) {
  // Startpage uses a path-based SERP URL that Chromium's query-based detection
  // does not support.
  EXPECT_TRUE(MaybeClassifySearchEngine(
      GURL(R"(https://www.startpage.com/sp/search)")));
}

TEST(SerpClassifierTest, DoNotClassifyNonSearchEngine) {
  EXPECT_FALSE(MaybeClassifySearchEngine(
      GURL(R"(https://www.perplexity.ai/search/new/foo)")));
  EXPECT_FALSE(MaybeClassifySearchEngine(GURL(R"(https://brave.com/)")));
  EXPECT_FALSE(MaybeClassifySearchEngine(GURL(R"(https://bar.com/baz)")));
  EXPECT_FALSE(
      MaybeClassifySearchEngine(GURL(R"(https://qux.quux.com/corge)")));
  EXPECT_FALSE(
      MaybeClassifySearchEngine(GURL(R"(https://startpage.com/grault)")));
  EXPECT_FALSE(
      MaybeClassifySearchEngine(GURL(R"(https://uk.search.yahoo.com/garply)")));
  EXPECT_FALSE(
      MaybeClassifySearchEngine(GURL(R"(https://search.yahoo.com/waldo)")));
}

}  // namespace serp_metrics
