/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_classifier.h"

#include <optional>

#include "base/containers/fixed_flat_set.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace serp_metrics {

namespace {

constexpr auto kAllowList = base::MakeFixedFlatSet<SearchEngineType>(
    base::sorted_unique,
    {SEARCH_ENGINE_BING, SEARCH_ENGINE_GOOGLE, SEARCH_ENGINE_YAHOO,
     SEARCH_ENGINE_DUCKDUCKGO, SEARCH_ENGINE_QWANT, SEARCH_ENGINE_ECOSIA,
     SEARCH_ENGINE_BRAVE, SEARCH_ENGINE_STARTPAGE});

void VerifySerpClassifierExpectation(
    const TemplateURLPrepopulateData::PrepopulatedEngine& prepopulated_engine) {
  const auto template_url_data =
      TemplateURLDataFromPrepopulatedEngine(prepopulated_engine);
  TemplateURL template_url(*template_url_data);

  GURL url = template_url.GenerateSearchURL(SearchTermsData(), u"test");
  ASSERT_TRUE(url.is_valid());

  SerpClassifier classifier;
  if (std::optional<SearchEngineType> search_engine_type =
          classifier.MaybeClassify(url)) {
    EXPECT_TRUE(kAllowList.contains(*search_engine_type));
  }
}

}  // namespace

TEST(SerpClassifierTest, IsSameSearchQuery) {
  SerpClassifier classifier;

  EXPECT_TRUE(classifier.IsSameSearchQuery(
      GURL(R"(https://www.qwant.com/?q=foobar)"),
      GURL(R"(https://www.qwant.com/?q=foobar&t=web)")));
}

TEST(SerpClassifierTest, IsSameSearchQueryWithDifferentParamOrder) {
  SerpClassifier classifier;

  EXPECT_TRUE(classifier.IsSameSearchQuery(
      GURL(R"(https://www.qwant.com/?q=foobar)"),
      GURL(R"(https://www.qwant.com/?t=web&q=foobar)")));
}

TEST(SerpClassifierTest, IsNotSameSearchQuery) {
  SerpClassifier classifier;

  EXPECT_FALSE(classifier.IsSameSearchQuery(
      GURL(R"(https://www.qwant.com/?q=foo&t=web)"),
      GURL(R"(https://www.qwant.com/?q=bar&t=web")")));
}

TEST(SerpClassifierTest, IsNotSameSearchQueryWithInvalidUrl) {
  SerpClassifier classifier;

  EXPECT_FALSE(classifier.IsSameSearchQuery(
      GURL(R"(https://www.qwant.com/?q=foobar)"), GURL("foobar")));
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

TEST(SerpClassifierTest, DoNotClassifyNonSearchEngine) {
  SerpClassifier classifier;

  EXPECT_FALSE(classifier.MaybeClassify(
      GURL(R"(https://www.perplexity.ai/search/new/foo)")));
  EXPECT_FALSE(classifier.MaybeClassify(GURL(R"(https://brave.com/)")));
  EXPECT_FALSE(classifier.MaybeClassify(GURL(R"(https://bar.com/baz)")));
  EXPECT_FALSE(classifier.MaybeClassify(GURL(R"(https://qux.quux.com/corge)")));
  EXPECT_FALSE(
      classifier.MaybeClassify(GURL(R"(https://startpage.com/grault)")));
  EXPECT_FALSE(
      classifier.MaybeClassify(GURL(R"(https://uk.search.yahoo.com/garply)")));
  EXPECT_FALSE(
      classifier.MaybeClassify(GURL(R"(https://search.yahoo.com/waldo)")));
}

}  // namespace serp_metrics
