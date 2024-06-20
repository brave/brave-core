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
#include "brave/components/constants/brave_paths.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "brave/components/web_discovery/common/web_discovery.mojom.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/re2/src/re2/re2.h"

namespace web_discovery {

class WebDiscoveryContentScraperTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    base::FilePath data_path =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    data_path = data_path.AppendASCII("web_discovery");

    host_resolver()->AddRule("*", "127.0.0.1");
    auto& test_server = embedded_https_test_server();
    test_server.ServeFilesFromDirectory(data_path);
    ASSERT_TRUE(test_server.Start());

    InitScraper();
    run_loop_ = std::make_unique<base::RunLoop>();

    ASSERT_TRUE(
        base::ReadFileToString(data_path.Append("page.html"), &page_content_));
  }

 protected:
  mojo::Remote<mojom::DocumentExtractor> LoadTestPageAndGetExtractor() {
    mojo::Remote<mojom::DocumentExtractor> remote;

    auto url = embedded_https_test_server().GetURL("example.com", "/page.html");
    auto* render_frame_host = ui_test_utils::NavigateToURL(browser(), url);

    if (render_frame_host) {
      render_frame_host->GetRemoteInterfaces()->GetInterface(
          remote.BindNewPipeAndPassReceiver());
    }
    return remote;
  }

  std::string page_content_;
  std::unique_ptr<ContentScraper> scraper_;
  std::unique_ptr<base::RunLoop> run_loop_;

 private:
  void InitScraper() {
    server_config_loader_ = std::make_unique<ServerConfigLoader>(
        nullptr, base::FilePath(), nullptr, base::DoNothing(),
        base::DoNothing());
    auto server_config = std::make_unique<ServerConfig>();
    server_config->location = "us";
    server_config_loader_->SetLastServerConfigForTest(std::move(server_config));

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
    scrape_rule4->functions_applied[0].push_back(base::Value("parseU"));
    scrape_rule4->functions_applied[0].push_back(base::Value("qs"));
    scrape_rule4->functions_applied[0].push_back(base::Value("q"));

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

    server_config_loader_->SetLastPatternsForTest(std::move(patterns_group));

    scraper_ = std::make_unique<ContentScraper>(server_config_loader_.get(),
                                                &regex_util_);
  }

  RegexUtil regex_util_;
  std::unique_ptr<ServerConfigLoader> server_config_loader_;
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

              auto field_map_it = scrape_result->fields.find("#b .result1");
              ASSERT_TRUE(field_map_it != scrape_result->fields.end());
              const auto* fields = &field_map_it->second;

              ASSERT_EQ(fields->size(), 2u);

              const auto* href_value = (*fields)[0].FindString("href");
              const auto* text_value = (*fields)[0].FindString("text");
              const auto* query_value_str = (*fields)[0].FindString("q");
              ASSERT_TRUE(href_value);
              ASSERT_TRUE(text_value);
              ASSERT_TRUE(query_value_str);
              EXPECT_EQ(*href_value, "https://example.com/foo1");
              EXPECT_EQ(*text_value, "Foo1");
              EXPECT_EQ(*query_value_str, "A query");

              href_value = (*fields)[1].FindString("href");
              text_value = (*fields)[1].FindString("text");
              const auto* query_value = (*fields)[1].Find("q");
              ASSERT_TRUE(href_value);
              ASSERT_TRUE(text_value);
              ASSERT_TRUE(query_value);
              EXPECT_EQ(*href_value, "https://example.com/foo2");
              EXPECT_EQ(*text_value, "Foo2");
              EXPECT_TRUE(query_value->is_none());

              field_map_it = scrape_result->fields.find("dont>match");
              ASSERT_TRUE(field_map_it != scrape_result->fields.end());
              fields = &field_map_it->second;

              ASSERT_EQ(fields->size(), 1u);
              const auto* url_query_value = (*fields)[0].FindString("q2");
              ASSERT_TRUE(url_query_value);
              EXPECT_EQ(*url_query_value, "testquery");
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

              auto field_map_it = scrape_result->fields.find("#b #result2");
              ASSERT_TRUE(field_map_it != scrape_result->fields.end());
              const auto* fields = &field_map_it->second;

              ASSERT_EQ(fields->size(), 1u);

              const auto* text_value = (*fields)[0].FindString("text");
              const auto* input_value = (*fields)[0].FindString("input");
              ASSERT_TRUE(text_value);
              ASSERT_TRUE(input_value);
              EXPECT_EQ(*text_value, "Foo3");
              EXPECT_EQ(*input_value, "Foo4");

              field_map_it = scrape_result->fields.find("dont>match");
              ASSERT_TRUE(field_map_it != scrape_result->fields.end());
              fields = &field_map_it->second;

              ASSERT_EQ(fields->size(), 1u);

              const auto* ctry_value = (*fields)[0].FindString("ctry");
              ASSERT_TRUE(ctry_value);
              EXPECT_EQ(*ctry_value, "us");
            }();
            run_loop_->Quit();
          }));
  run_loop_->Run();
}

}  // namespace web_discovery
