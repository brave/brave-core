/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/content_scraper.h"

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "brave/components/web_discovery/browser/patterns_v2.h"
#include "brave/components/web_discovery/browser/relevant_site.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "brave/components/web_discovery/browser/url_extractor.h"
#include "brave/components/web_discovery/common/features.h"
#include "brave/components/web_discovery/common/web_discovery.mojom.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_base.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/re2/src/re2/re2.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

using base::test::IsSupersetOfValue;

namespace web_discovery {

class WebDiscoveryContentScraperTest : public PlatformBrowserTest {
 public:
  WebDiscoveryContentScraperTest()
      : scoped_features_(features::kBraveWebDiscoveryNative) {}

  // PlatformBrowserTest:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    base::FilePath data_path =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    data_path = data_path.AppendASCII("web_discovery");

    host_resolver()->AddRule("*", "127.0.0.1");
    test_server_.ServeFilesFromDirectory(data_path);
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    ASSERT_TRUE(test_server_.Start());

    InitServerConfig();
    InitScraper();
    run_loop_ = std::make_unique<base::RunLoop>();

    ASSERT_TRUE(base::ReadFileToString(data_path.AppendASCII("page.html"),
                                       &page_content_));
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

 protected:
  mojo::Remote<mojom::DocumentExtractor> LoadTestPageAndGetExtractor() {
    mojo::Remote<mojom::DocumentExtractor> remote;

    auto url = test_server_.GetURL("example.com", "/page.html");
    auto* contents = chrome_test_utils::GetActiveWebContents(this);
    EXPECT_TRUE(content::NavigateToURL(contents, url));
    auto* render_frame_host = contents->GetPrimaryMainFrame();

    if (render_frame_host) {
      render_frame_host->GetRemoteInterfaces()->GetInterface(
          remote.BindNewPipeAndPassReceiver());
    }
    return remote;
  }

  void InitServerConfig() {
    server_config_loader_ = std::make_unique<ServerConfigLoader>(
        nullptr, base::FilePath(), nullptr, base::DoNothing(),
        base::DoNothing());
    auto server_config = std::make_unique<ServerConfig>();
    server_config->location = "us";
    server_config_loader_->SetLastServerConfigForTesting(
        std::move(server_config));
  }

  void InitScraper() {
    auto patterns_group = std::make_unique<PatternsGroup>();
    std::vector<PatternsURLDetails> normal_patterns(1);
    std::vector<PatternsURLDetails> strict_patterns(1);
    normal_patterns[0].url_regex =
        std::make_unique<re2::RE2>("^https:\\/\\/example\\.com");
    normal_patterns[0].id = "ex1";
    strict_patterns[0].url_regex =
        std::make_unique<re2::RE2>("^https:\\/\\/example\\.com");
    strict_patterns[0].id = "ex1";

    auto* normal_rule_group =
        &normal_patterns[0].scrape_rule_groups["#b .result1"];
    auto scrape_rule1 = std::make_unique<ScrapeRule>();
    scrape_rule1->sub_selector = "a";
    scrape_rule1->rule_type = ScrapeRuleType::kOther;
    scrape_rule1->attribute = "href";
    normal_rule_group->insert_or_assign("href", std::move(scrape_rule1));
    auto scrape_rule2 = std::make_unique<ScrapeRule>();
    scrape_rule2->sub_selector = "a";
    scrape_rule2->rule_type = ScrapeRuleType::kOther;
    scrape_rule2->attribute = "textContent";
    normal_rule_group->insert_or_assign("text", std::move(scrape_rule2));
    auto scrape_rule3 = std::make_unique<ScrapeRule>();
    scrape_rule3->sub_selector = "#query";
    scrape_rule3->rule_type = ScrapeRuleType::kSearchQuery;
    scrape_rule3->attribute = "textContent";
    normal_rule_group->insert_or_assign("q", std::move(scrape_rule3));
    normal_rule_group = &normal_patterns[0].scrape_rule_groups["dont>match"];
    auto scrape_rule4 = std::make_unique<ScrapeRule>();
    scrape_rule4->rule_type = ScrapeRuleType::kStandard;
    scrape_rule4->attribute = "url";
    scrape_rule4->functions_applied.emplace_back();
    scrape_rule4->functions_applied[0].Append(base::Value("parseU"));
    scrape_rule4->functions_applied[0].Append(base::Value("qs"));
    scrape_rule4->functions_applied[0].Append(base::Value("q"));

    normal_rule_group->insert_or_assign("q2", std::move(scrape_rule4));

    patterns_group->normal_patterns = std::move(normal_patterns);

    auto* strict_rule_group =
        &strict_patterns[0].scrape_rule_groups["#b #result2"];
    scrape_rule1 = std::make_unique<ScrapeRule>();
    scrape_rule1->sub_selector = "a";
    scrape_rule1->rule_type = ScrapeRuleType::kOther;
    scrape_rule1->attribute = "textContent";
    strict_rule_group->insert_or_assign("text", std::move(scrape_rule1));
    scrape_rule2 = std::make_unique<ScrapeRule>();
    scrape_rule2->sub_selector = "#input1";
    scrape_rule2->rule_type = ScrapeRuleType::kOther;
    scrape_rule2->attribute = "value";
    strict_rule_group->insert_or_assign("input", std::move(scrape_rule2));
    strict_rule_group = &strict_patterns[0].scrape_rule_groups["dont>match"];
    scrape_rule3 = std::make_unique<ScrapeRule>();
    scrape_rule3->rule_type = ScrapeRuleType::kStandard;
    scrape_rule3->attribute = "ctry";
    strict_rule_group->insert_or_assign("ctry", std::move(scrape_rule3));

    patterns_group->strict_patterns = std::move(strict_patterns);

    server_config_loader_->SetLastPatternsForTesting(std::move(patterns_group));

    scraper_ = ContentScraper::Create(server_config_loader_.get(), nullptr);
  }

  void InitScraperV2() {
    auto v2_patterns_group = std::make_unique<V2PatternsGroup>();

    // Create a test V2 site pattern for example.com
    V2SitePattern site_pattern;

    // Create input group for CSS selector
    V2InputGroup input_group;
    input_group.select_all = true;

    // Create extraction rule for href attribute
    V2ExtractionRule href_rule;
    href_rule.sub_selector = "a";
    href_rule.attribute = "href";

    // Create extraction rule for text content
    V2ExtractionRule text_rule;
    text_rule.sub_selector = "a";
    text_rule.attribute = "textContent";

    // Add rules to input group
    input_group.extraction_rules["url"].push_back(std::move(href_rule));
    input_group.extraction_rules["text"].push_back(std::move(text_rule));

    // Add input group to site pattern
    site_pattern.input_groups["#b .result1"] = std::move(input_group);

    // Create input group for #result2 (without select_all)
    V2InputGroup result2_input_group;
    result2_input_group.select_all = false;

    // Create an extraction rule. This should be skipped
    // because the element should not exist.
    V2ExtractionRule input_value_rule1;
    input_value_rule1.sub_selector = "div";
    input_value_rule1.attribute = "style";

    // Create an extraction rule for input value that works
    V2ExtractionRule input_value_rule2;
    input_value_rule2.sub_selector = "input";
    input_value_rule2.attribute = "value";

    // Add rule to input group
    result2_input_group.extraction_rules["input_value"].push_back(
        std::move(input_value_rule1));
    result2_input_group.extraction_rules["input_value"].push_back(
        std::move(input_value_rule2));

    // Add result2 input group to site pattern
    site_pattern.input_groups["#result2"] = std::move(result2_input_group);

    // Add site pattern to V2 patterns group
    v2_patterns_group->site_patterns[RelevantSite::kBing] =
        std::move(site_pattern);

    // Set the V2 patterns for testing
    server_config_loader_->SetLastV2PatternsForTesting(
        std::move(v2_patterns_group));

    url_extractor_ = std::make_unique<URLExtractor>();
    scraper_ = ContentScraper::Create(server_config_loader_.get(),
                                      url_extractor_.get());
  }

  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer test_server_{net::EmbeddedTestServer::TYPE_HTTPS};
  base::test::ScopedFeatureList scoped_features_;
  std::unique_ptr<ServerConfigLoader> server_config_loader_;
  std::unique_ptr<URLExtractor> url_extractor_;

  std::string page_content_;
  std::unique_ptr<ContentScraper> scraper_;
  std::unique_ptr<base::RunLoop> run_loop_;
};

IN_PROC_BROWSER_TEST_F(WebDiscoveryContentScraperTest, RendererScrape) {
  auto extractor = LoadTestPageAndGetExtractor();
  ASSERT_TRUE(extractor.is_bound() && extractor.is_connected());

  GURL url("https://example.com/page?q=testquery");
  scraper_->ScrapePage(
      url, false, extractor.get(),
      base::BindLambdaForTesting(
          [&](std::unique_ptr<PageScrapeResult> scrape_result) {
            [&] {
              ASSERT_TRUE(scrape_result);
              EXPECT_EQ(scrape_result->url, url);
              EXPECT_EQ(scrape_result->fields.size(), 2u);
              EXPECT_EQ(scrape_result->id, "ex1");

              EXPECT_EQ(scrape_result->query, "A query");

              const auto* fields =
                  base::FindOrNull(scrape_result->fields, "#b .result1");
              ASSERT_TRUE(fields);

              ASSERT_EQ(fields->size(), 2u);

              EXPECT_THAT(
                  (*fields)[0],
                  IsSupersetOfValue(base::Value::Dict()
                                        .Set("href", "https://example.com/foo1")
                                        .Set("text", "Foo1")
                                        .Set("q", "A query")));

              EXPECT_THAT(
                  (*fields)[1],
                  IsSupersetOfValue(base::Value::Dict()
                                        .Set("href", "https://example.com/foo2")
                                        .Set("text", "Foo2")
                                        .Set("q", base::Value())));

              fields = base::FindOrNull(scrape_result->fields, "dont>match");
              ASSERT_TRUE(fields);

              ASSERT_EQ(fields->size(), 1u);
              EXPECT_THAT((*fields)[0],
                          IsSupersetOfValue(
                              base::Value::Dict().Set("q2", "testquery")));
            }();
            run_loop_->Quit();
          }));
  run_loop_->Run();
}

IN_PROC_BROWSER_TEST_F(WebDiscoveryContentScraperTest, RustParseAndScrape) {
  GURL url("https://example.com/page.html");

  auto prev_scrape_result = std::make_unique<PageScrapeResult>(url, "ex1");
  scraper_->ParseAndScrapePage(
      url, true, std::move(prev_scrape_result), page_content_,
      base::BindLambdaForTesting(
          [&](std::unique_ptr<PageScrapeResult> scrape_result) {
            [&] {
              ASSERT_TRUE(scrape_result);
              EXPECT_EQ(scrape_result->url, url);
              EXPECT_EQ(scrape_result->fields.size(), 2u);
              EXPECT_EQ(scrape_result->id, "ex1");

              EXPECT_FALSE(scrape_result->query);

              const auto* fields =
                  base::FindOrNull(scrape_result->fields, "#b #result2");
              ASSERT_TRUE(fields);

              ASSERT_EQ(fields->size(), 1u);

              EXPECT_THAT((*fields)[0],
                          IsSupersetOfValue(base::Value::Dict()
                                                .Set("text", "Foo3")
                                                .Set("input", "Foo4")));

              fields = base::FindOrNull(scrape_result->fields, "dont>match");
              ASSERT_TRUE(fields);

              ASSERT_EQ(fields->size(), 1u);
              EXPECT_THAT(
                  (*fields)[0],
                  IsSupersetOfValue(base::Value::Dict().Set("ctry", "us")));
            }();
            run_loop_->Quit();
          }));
  run_loop_->Run();
}

IN_PROC_BROWSER_TEST_F(WebDiscoveryContentScraperTest, RustParseAndScrapeV2) {
  // Reset scraper to use V2 patterns and URL extractor
  InitScraperV2();

  GURL url("https://www.bing.com/search?q=apple%20types");

  scraper_->ParseAndScrapePageV2(
      url, page_content_,
      base::BindLambdaForTesting(
          [&](std::unique_ptr<PageScrapeResult> scrape_result) {
            [&] {
              ASSERT_TRUE(scrape_result);
              EXPECT_EQ(scrape_result->url, url);

              // V2 parsing should extract from #b .result1 elements
              const auto* fields =
                  base::FindOrNull(scrape_result->fields, "#b .result1");
              ASSERT_TRUE(fields);

              // Should have extracted 2 results from the .result1 elements
              ASSERT_EQ(fields->size(), 2u);

              EXPECT_THAT(
                  (*fields)[0],
                  IsSupersetOfValue(base::Value::Dict()
                                        .Set("url", "https://example.com/foo1")
                                        .Set("text", "Foo1")));

              EXPECT_THAT(
                  (*fields)[1],
                  IsSupersetOfValue(base::Value::Dict()
                                        .Set("url", "https://example.com/foo2")
                                        .Set("text", "Foo2")));

              // Check #result2 extraction (select_all = false, so single
              // result)
              fields = base::FindOrNull(scrape_result->fields, "#result2");
              ASSERT_TRUE(fields);
              ASSERT_EQ(fields->size(), 1u);

              // Should extract input value "Foo4"
              EXPECT_THAT((*fields)[0],
                          IsSupersetOfValue(
                              base::Value::Dict().Set("input_value", "Foo4")));
            }();
            run_loop_->Quit();
          }));
  run_loop_->Run();
}

}  // namespace web_discovery
