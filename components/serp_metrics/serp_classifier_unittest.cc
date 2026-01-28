/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_classifier.h"

#include <memory>

#include "base/check.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/search_engines_test_environment.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace metrics {

namespace {

TemplateURL* FindByPrepopulateId(TemplateURLService* template_url_service,
                                 int id) {
  CHECK(template_url_service);

  for (TemplateURL* template_url : template_url_service->GetTemplateURLs()) {
    if (template_url && template_url->prepopulate_id() == id) {
      return template_url;
    }
  }

  return nullptr;
}

// TODO(tmancey): Investigate using the real `TemplateURLService` prepopulation
// logic in tests. Discussing with @bridiver.
void PrepopulateTemplateURLService(TemplateURLService* template_url_service) {
  CHECK(template_url_service);

  // Add all prepopulated engines from Chromium first.
  for (const TemplateURLPrepopulateData::PrepopulatedEngine*
           prepopulated_engine : TemplateURLPrepopulateData::kAllEngines) {
    CHECK(prepopulated_engine);

    if (FindByPrepopulateId(template_url_service, prepopulated_engine->id)) {
      continue;
    }

    std::unique_ptr<TemplateURLData> template_url_data =
        TemplateURLDataFromPrepopulatedEngine(*prepopulated_engine);
    CHECK(template_url_data);
    template_url_service->Add(
        std::make_unique<TemplateURL>(*template_url_data));
  }

  // Now add Brave-specific prepopulated engines.
  for (const auto& [id, prepopulated_engine] :
       TemplateURLPrepopulateData::kBraveEngines) {
    CHECK(prepopulated_engine);

    if (FindByPrepopulateId(template_url_service, prepopulated_engine->id)) {
      continue;
    }

    std::unique_ptr<TemplateURLData> template_url_data =
        TemplateURLDataFromPrepopulatedEngine(*prepopulated_engine);
    CHECK(template_url_data);
    template_url_service->Add(
        std::make_unique<TemplateURL>(*template_url_data));
  }
}

}  // namespace

class SerpClassifierTest : public testing::Test {
 protected:
  void SetUp() override {
    template_url_service_ =
        search_engines_test_environment_.template_url_service();
    ASSERT_TRUE(template_url_service_);

    template_url_service_->Load();
    ASSERT_TRUE(template_url_service_->loaded());

    PrepopulateTemplateURLService(template_url_service_);
  }

  TemplateURLService* template_url_service() { return template_url_service_; }

 private:
  base::test::SingleThreadTaskEnvironment task_environment_;
  search_engines::SearchEnginesTestEnvironment search_engines_test_environment_;
  raw_ptr<TemplateURLService> template_url_service_ = nullptr;
};

TEST_F(SerpClassifierTest, ClassifyQueryBasedTemplateUrlSearchEngines) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kSerpMetricsFeature);

  SerpClassifier classifier(template_url_service());

  /* Brave */
  EXPECT_EQ(SearchEngineType::SEARCH_ENGINE_BRAVE,
            classifier.Classify(GURL(
                R"(https://search.brave.com/search?q=foobar&source=web)")));

  /* Google */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_GOOGLE,
      classifier.Classify(GURL(
          R"(https://www.google.com/search?q=foobar&sca_esv=76156e36b6817723&sxsrf=ANbL-n6QnD8wxx3-mGlxNR8KIZSDgUYRPA%3A1769199851341&source=hp&ei=69hzafnxEtqqwbkPwuva8Qg&iflsig=AFdpzrgAAAAAaXPm-8WLXmtp5HpWJECsjI8tX_VK1brF&ved=0ahUKEwi5k7KFv6KSAxVaVTABHcK1No4Q4dUDCCA&uact=5&oq=foobar&gs_lp=Egdnd3Mtd2l6IgZmb29iYXIyBRAAGIAEMgUQABiABDIPEAAYgAQYsQMYgwEYChgLMg8QABiABBixAxiDARgKGAsyBRAAGIAEMg8QABiABBixAxiDARgKGAsyBRAAGIAEMgwQABiABBixAxgKGAsyDxAAGIAEGLEDGIMBGAoYCzIQEAAYgAQYsQMYgwEYigUYCkjgBVBEWMYFcAF4AJABAJgBZqAB4AKqAQMzLjG4AQPIAQD4AQGYAgWgAu8CqAIKwgIKEAAYAxjqAhiPAcICChAuGAMY6gIYjwHCAhEQLhiABBixAxjRAxiDARjHAcICCxAAGIAEGLEDGIMBwgIUEC4YgAQYsQMYgwEYxwEYigUYrwHCAggQLhiABBixA8ICCBAAGIAEGLEDwgIOEAAYgAQYsQMYgwEYigXCAgUQLhiABMICCxAuGIAEGNEDGMcBwgIOEC4YgAQYsQMY0QMYxwGYAwTxBfuh5nekEME-kgcDNC4xoAezJLIHAzMuMbgH6wLCBwUwLjQuMcgHC4AIAA&sclient=gws-wiz)")));

  /* DuckDuckGo */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_DUCKDUCKGO,
      classifier.Classify(GURL(
          R"(https://duckduckgo.com/?ia=web&origin=funnel_home_website&t=h_&q=foobar&chip-select=search)")));

  /* Qwant */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_QWANT,
      classifier.Classify(GURL(R"(https://www.qwant.com/?q=foobar&t=web)")));

  /* Bing */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_BING,
      classifier.Classify(GURL(
          R"(https://www.bing.com/search?q=foobar&form=QBLH&sp=-1&ghc=1&lq=0&pq=fooba&sc=12-5&qs=n&sk=&cvid=227EE41587C74448ACE88DFBD62B5E3F)")));

  /* Ecosia */
  EXPECT_EQ(SearchEngineType::SEARCH_ENGINE_ECOSIA,
            classifier.Classify(GURL(
                R"(https://www.ecosia.org/search?method=index&q=foobar)")));

  /* Daum */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_DAUM,
      classifier.Classify(GURL(
          R"(https://search.daum.net/search?w=tot&DA=YZR&t__nil_searchbox=btn&q=foobar)")));

  /* Freespoke */
  EXPECT_EQ(SearchEngineType::SEARCH_ENGINE_FREESPOKE,
            classifier.Classify(
                GURL(R"(https://freespoke.com/search/web?q=foobar)")));

  /* Info.com */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_INFO_COM,
      classifier.Classify(GURL(
          R"(https://www.info.com/serp?q=foobar&segment=info.infous.udog2)")));

  /* Kagi */
  EXPECT_EQ(SearchEngineType::SEARCH_ENGINE_KAGI,
            classifier.Classify(GURL(R"(https://kagi.com/search?q=foobar)")));

  /* Lilo */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_LILO,
      classifier.Classify(GURL(R"(https://search.lilo.org/?q=foobar&t=web)")));

  /* Mojeek */
  EXPECT_EQ(SearchEngineType::SEARCH_ENGINE_MOJEEK,
            classifier.Classify(
                GURL(R"(https://www.mojeek.com/search?q=foobar&theme=dark)")));

  /* Naver */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_NAVER,
      classifier.Classify(GURL(
          R"(https://search.naver.com/search.naver?where=nexearch&sm=top_hty&fbm=0&ie=utf8&query=foobar&ackey=c1hehqf3)")));

  /* Nona */
  EXPECT_EQ(SearchEngineType::SEARCH_ENGINE_NONA,
            classifier.Classify(GURL(R"(https://www.nona.de/?q=foobar)")));

  /* Seznam */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_SEZNAM,
      classifier.Classify(GURL(
          R"(https://search.seznam.cz/?q=foobar&oq=foobar&aq=-1&ms=1960&ks=12&sourceid=trending-hp&sId=QfoZcwCDZlMUQpj9Wg2d)")));

  /* 360 Search */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_360,
      classifier.Classify(GURL(
          R"(https://www.so.com/s?q=foobar&src=360sou_newhome&ssid=8ea5e221c71149978377a0785867d4ac&sp=a56&cp=0ef00452b0&nlpv=global_place_c_shyc&fr=360sou_newhome)")));

  /* Sogou */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_SOGOU,
      classifier.Classify(GURL(
          R"(https://www.sogou.com/web?query=foobar&_asf=www.sogou.com&_ast=&w=01019900&p=40040100&ie=utf8&from=index-nologin&s_from=index&sourceid=9_01_03&sessiontime=1769199046301)")));

  /* Yep */
  EXPECT_EQ(SearchEngineType::SEARCH_ENGINE_YEP,
            classifier.Classify(GURL(R"(https://yep.com/web?q=foobar)")));

  /* You.com */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_YOU,
      classifier.Classify(GURL(
          R"(https://you.com/search?q=foobar&fromSearchBar=true&chatMode=default)")));
}

TEST_F(SerpClassifierTest, ClassifyPathBasedUrlSearchEngines) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kSerpMetricsFeature);

  SerpClassifier classifier(template_url_service());

  /* ChatGPT */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_OTHER,
      classifier.Classify(GURL(
          R"(https://chatgpt.com/c/6973d556-c39c-832f-9803-05faf65e660e)")));

  /* Perplexity AI */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_OTHER,
      classifier.Classify(GURL(
          R"(https://www.perplexity.ai/search/foobar-qPy_CBr.QsaCii9lFf22Pw)")));

  /* Yahoo */
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_YAHOO,
      classifier.Classify(GURL(
          R"(https://mx.search.yahoo.com/search;_ylt=AwrFNvqI2HNpZgwMczTC8Qt.;_ylc=X1MDMjExNDcxMjAwMgRfcgMyBGZyAwRmcjIDcDpzLHY6c2ZwLG06c2ItdG9wBGdwcmlkA01NdW1QY0h5VEMyWW5qb0FsbWpxbkEEbl9yc2x0AzAEbl9zdWdnAzkEb3JpZ2luA214LnNlYXJjaC55YWhvby5jb20EcG9zAzAEcHFzdHIDBHBxc3RybAMwBHFzdHJsAzYEcXVlcnkDZm9vYmFyBHRfc3RtcAMxNzY5MTk5NzU0?p=foobar&fr=sfp&fr2=p%3As%2Cv%3Asfp%2Cm%3Asb-top&iscqry=)")));
  EXPECT_EQ(
      SearchEngineType::SEARCH_ENGINE_YAHOO,
      classifier.Classify(GURL(
          R"(https://search.yahoo.com/search;_ylt=AwrErm2d2HNpHR0MPvdDDWVH;_ylc=X1MDMTE5NzgwNDg2NwRfcgMyBGZyAwRmcjIDcDpzLHY6c2ZwLG06c2ItdG9wBGdwcmlkA0VFNE4wOUZIU2FlQlBVcmQ3M1EyTUEEbl9yc2x0AzAEbl9zdWdnAzEwBG9yaWdpbgNzZWFyY2gueWFob28uY29tBHBvcwMwBHBxc3RyAwRwcXN0cmwDMARxc3RybAM2BHF1ZXJ5A2Zvb2JhcgR0X3N0bXADMTc2OTE5OTc3NQ--?p=foobar&fr=sfp&fr2=p%3As%2Cv%3Asfp%2Cm%3Asb-top&iscqry=)")));

  /* Startpage */
  EXPECT_EQ(SearchEngineType::SEARCH_ENGINE_STARTPAGE,
            classifier.Classify(GURL(R"(https://startpage.com/sp/search)")));
}

TEST_F(SerpClassifierTest, DoNotClassifyDisallowedSearchEngines) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kSerpMetricsFeature);

  SerpClassifier classifier(template_url_service());

  EXPECT_FALSE(classifier.Classify(GURL(R"(chrome://bookmarks/?q=foobar)")));
  EXPECT_FALSE(classifier.Classify(GURL(R"(https://www.google.com/)")));
}

TEST_F(SerpClassifierTest, DoNotClassifyNonSearchEngine) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kSerpMetricsFeature);

  SerpClassifier classifier(template_url_service());

  EXPECT_FALSE(
      classifier.Classify(GURL(R"(https://www.perplexity.ai/search/new/foo)")));
  EXPECT_FALSE(classifier.Classify(GURL(R"(https://chatgpt.com/)")));
  EXPECT_FALSE(classifier.Classify(GURL(R"(https://bar.com/baz)")));
  EXPECT_FALSE(classifier.Classify(GURL(R"(https://qux.quux.com/corge)")));
  EXPECT_FALSE(classifier.Classify(GURL(R"(https://startpage.com/grault)")));
  EXPECT_FALSE(
      classifier.Classify(GURL(R"(https://uk.search.yahoo.com/garply)")));
  EXPECT_FALSE(classifier.Classify(GURL(R"(https://search.yahoo.com/waldo)")));
}

TEST_F(SerpClassifierTest, DoNotClassifyIfTemplateUrlServiceIsNull) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kSerpMetricsFeature);

  SerpClassifier classifier(/*template_url_service=*/nullptr);

  EXPECT_FALSE(classifier.Classify(
      GURL(R"(https://search.brave.com/search?q=foobar&source=web)")));
  EXPECT_FALSE(classifier.Classify(
      GURL(R"(https://chatgpt.com/c/6973d556-c39c-832f-9803-05faf65e660e)")));
}

TEST_F(SerpClassifierTest, DoNotClassifyIfSerpMetricsIsDisabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kSerpMetricsFeature);

  SerpClassifier classifier(template_url_service());

  EXPECT_FALSE(classifier.Classify(
      GURL(R"(https://search.brave.com/search?q=foobar&source=web)")));
}

TEST_F(SerpClassifierTest, DoNotReclassifySameSerpUrlConsecutively) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kSerpMetricsFeature);

  SerpClassifier classifier(template_url_service());

  const GURL url(R"(https://www.qwant.com/?q=foobar&t=web)");
  ASSERT_EQ(SearchEngineType::SEARCH_ENGINE_QWANT, classifier.Classify(url));

  // Same URL should be deduped.
  EXPECT_FALSE(classifier.Classify(url));
}

}  // namespace metrics
