/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/content_scraper.h"

#include <utility>

#include "third_party/re2/src/re2/re2.h"

namespace web_discovery {

constexpr char kUrlAttrId[] = "url";

PageScrapeResult::PageScrapeResult() = default;
PageScrapeResult::~PageScrapeResult() = default;

ContentScraper::ContentScraper(std::unique_ptr<PatternsGroup>* patterns)
    : patterns_(patterns) {}

ContentScraper::~ContentScraper() = default;

const PatternsURLDetails* ContentScraper::GetMatchingURLPattern(
    const GURL& url,
    bool is_strict_scrape) {
  const auto& patterns = is_strict_scrape ? patterns_->get()->strict_patterns
                                          : patterns_->get()->normal_patterns;
  for (const auto& pattern : patterns) {
    if (re2::RE2::PartialMatch(url.spec(), *pattern.url_regex) &&
        !pattern.scrape_rule_groups.empty()) {
      return &pattern;
    }
  }
  return nullptr;
}

void ContentScraper::ScrapePage(const PatternsURLDetails* url_details,
                                const GURL& url,
                                mojom::DocumentExtractor* document_extractor,
                                PageScrapeResultCallback callback) {
  auto interim_result = std::make_unique<PageScrapeResult>();
  interim_result->id = url_details->id;

  std::vector<mojom::SelectRequestPtr> select_requests;
  for (const auto& group : url_details->scrape_rule_groups) {
    auto select_request = mojom::SelectRequest::New();
    select_request->root_selector = group.selector;
    for (const auto& rule : group.rules) {
      if (rule.rule_type == ScrapeRuleType::kStandard) {
        if (rule.attribute == kUrlAttrId) {
          interim_result->fields.insert_or_assign(
              rule.report_key, std::vector<std::string>({url.spec()}));
        }
      } else {
        auto attribute_request = mojom::SelectAttributeRequest::New();
        attribute_request->sub_selector = rule.sub_selector;
        attribute_request->attribute = rule.attribute;
        attribute_request->key = rule.report_key;

        select_request->attribute_requests.push_back(
            std::move(attribute_request));
      }
    }
    select_requests.push_back(std::move(select_request));
  }

  document_extractor->QueryElementAttributes(
      std::move(select_requests),
      base::BindOnce(&ContentScraper::OnElementAttributes,
                     weak_ptr_factory_.GetWeakPtr(), std::move(interim_result),
                     std::move(callback)));
}

void ContentScraper::OnElementAttributes(
    std::unique_ptr<PageScrapeResult> scrape_result,
    PageScrapeResultCallback callback,
    std::vector<mojom::AttributeResultPtr> attribute_results) {
  for (const auto& attribute_result : attribute_results) {
    scrape_result->fields.insert_or_assign(attribute_result->key,
                                           attribute_result->attribute_values);
  }
  std::move(callback).Run(std::move(scrape_result));
}

}  // namespace web_discovery
