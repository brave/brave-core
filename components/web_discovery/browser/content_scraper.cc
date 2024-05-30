/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/content_scraper.h"

#include <utility>

#include "base/ranges/algorithm.h"
#include "base/task/thread_pool.h"
#include "third_party/re2/src/re2/re2.h"

namespace web_discovery {

namespace {

constexpr char kUrlAttrId[] = "url";
constexpr char kFieldsValueKey[] = "fields";
constexpr char kIdValueKey[] = "id";
constexpr char kUrlValueKey[] = "url";

}  // namespace

PageScrapeResult::PageScrapeResult(GURL url, std::string id)
    : url(url), id(id) {}
PageScrapeResult::~PageScrapeResult() = default;

ContentScraper::ContentScraper(std::unique_ptr<PatternsGroup>* patterns)
    : pool_sequenced_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({})),
      patterns_(patterns) {}

ContentScraper::~ContentScraper() = default;

base::Value PageScrapeResult::SerializeToValue() {
  base::Value::Dict result;
  base::Value::Dict fields_dict;

  for (const auto& [key, values] : fields) {
    base::Value::List list;
    for (const auto& value : values) {
      list.Append(value);
    }
    fields_dict.Set(key, std::move(list));
  }

  result.Set(kFieldsValueKey, std::move(fields_dict));
  result.Set(kIdValueKey, id);
  result.Set(kUrlValueKey, url.spec());
  return base::Value(std::move(result));
}

std::unique_ptr<PageScrapeResult> PageScrapeResult::FromValue(
    const base::Value& value) {
  if (!value.is_dict()) {
    return nullptr;
  }
  const auto& dict = value.GetDict();
  const auto* fields_dict = dict.FindDict(kFieldsValueKey);
  const auto* id = dict.FindString(kIdValueKey);
  const auto* url = dict.FindString(kUrlValueKey);

  if (!fields_dict || !id || !url) {
    return nullptr;
  }

  auto result = std::make_unique<PageScrapeResult>(GURL(*url), *id);
  for (const auto [key, values] : *fields_dict) {
    std::vector<std::string> values_vec;
    for (const auto& val : values.GetList()) {
      if (!val.is_string()) {
        return nullptr;
      }
      values_vec.push_back(val.GetString());
    }
    result->fields[key] = std::move(values_vec);
  }

  return result;
}

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
  auto interim_result =
      std::make_unique<PageScrapeResult>(url, url_details->id);

  std::vector<mojom::SelectRequestPtr> select_requests;
  for (const auto& group : url_details->scrape_rule_groups) {
    auto select_request = mojom::SelectRequest::New();
    select_request->root_selector = group.selector;
    for (const auto& rule : group.rules) {
      if (rule.rule_type == ScrapeRuleType::kStandard) {
        ProcessStandardRule(rule, url, interim_result.get());
        continue;
      }
      auto attribute_request = mojom::SelectAttributeRequest::New();
      attribute_request->sub_selector = rule.sub_selector;
      attribute_request->attribute = rule.attribute;
      attribute_request->key = rule.report_key;

      select_request->attribute_requests.push_back(
          std::move(attribute_request));
    }
    select_requests.push_back(std::move(select_request));
  }

  document_extractor->QueryElementAttributes(
      std::move(select_requests),
      base::BindOnce(&ContentScraper::OnScrapedElementAttributes,
                     base::Unretained(this), std::move(interim_result),
                     std::move(callback)));
}

void ContentScraper::ParseAndScrapePage(
    std::unique_ptr<PageScrapeResult> prev_result,
    const PatternsURLDetails* url_details,
    std::string html,
    PageScrapeResultCallback callback) {
  auto interim_result = std::move(prev_result);

  std::vector<rust_document_extractor::SelectRequest> select_requests;
  for (const auto& group : url_details->scrape_rule_groups) {
    rust_document_extractor::SelectRequest select_request;
    select_request.root_selector = group.selector;
    for (const auto& rule : group.rules) {
      if (rule.rule_type == ScrapeRuleType::kStandard) {
        ProcessStandardRule(rule, interim_result->url, interim_result.get());
        continue;
      }
      rust_document_extractor::SelectAttributeRequest attribute_request{
          .sub_selector = rule.sub_selector.value_or(""),
          .key = rule.report_key,
          .attribute = rule.attribute,
      };
      select_request.attribute_requests.push_back(std::move(attribute_request));
    }
    select_requests.push_back(std::move(select_request));
  }

  pool_sequenced_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&rust_document_extractor::query_element_attributes, html,
                     select_requests),
      base::BindOnce(&ContentScraper::OnRustElementAttributes,
                     weak_ptr_factory_.GetWeakPtr(), std::move(interim_result),
                     std::move(callback)));
}

void ContentScraper::ProcessStandardRule(const ScrapeRule& rule,
                                         const GURL& url,
                                         PageScrapeResult* scrape_result) {
  if (rule.attribute == kUrlAttrId) {
    scrape_result->fields.insert_or_assign(
        rule.report_key, std::vector<std::string>({url.spec()}));
  }
}

void ContentScraper::OnScrapedElementAttributes(
    std::unique_ptr<PageScrapeResult> scrape_result,
    PageScrapeResultCallback callback,
    std::vector<mojom::AttributeResultPtr> attribute_results) {
  for (const auto& attribute_result : attribute_results) {
    scrape_result->fields.insert_or_assign(attribute_result->key,
                                           attribute_result->attribute_values);
  }
  std::move(callback).Run(std::move(scrape_result));
}

void ContentScraper::OnRustElementAttributes(
    std::unique_ptr<PageScrapeResult> scrape_result,
    PageScrapeResultCallback callback,
    rust::Vec<rust_document_extractor::AttributeResult> attribute_results) {
  for (const auto& attribute_result : attribute_results) {
    std::vector<std::string> attribute_values;
    base::ranges::transform(attribute_result.attribute_values.begin(),
                            attribute_result.attribute_values.end(),
                            std::back_inserter(attribute_values),
                            [](auto value) { return std::string(value); });
    scrape_result->fields.insert_or_assign(std::string(attribute_result.key),
                                           attribute_values);
  }
  std::move(callback).Run(std::move(scrape_result));
}

}  // namespace web_discovery
