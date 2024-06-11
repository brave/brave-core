/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/content_scraper.h"

#include <utility>

#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "url/url_util.h"

namespace web_discovery {

namespace {

constexpr char kUrlAttrId[] = "url";
constexpr char kCountryCodeAttrId[] = "ctry";
constexpr char kFieldsValueKey[] = "fields";
constexpr char kIdValueKey[] = "id";
constexpr char kUrlValueKey[] = "url";

constexpr char kRefineSplitFuncId[] = "splitF";

std::string RefineSplit(const std::string& value,
                        const std::string& delimiter,
                        int index) {
  auto split = base::SplitStringUsingSubstr(
      value, delimiter, base::WhitespaceHandling::KEEP_WHITESPACE,
      base::SPLIT_WANT_ALL);
  std::string encoded_result;
  if (index < 0 || static_cast<size_t>(index) >= split.size()) {
    encoded_result = value;
  } else {
    encoded_result = split[index];
  }
  url::RawCanonOutputT<char16_t> result;
  url::DecodeURLEscapeSequences(encoded_result,
                                url::DecodeURLMode::kUTF8OrIsomorphic, &result);
  return base::UTF16ToUTF8(result.view());
}

std::string ExecuteRefineFunctions(const RefineFunctionList& function_list,
                                   const std::string& value) {
  std::string result = value;
  for (const auto& function_args : function_list) {
    if (function_args.empty()) {
      continue;
    }
    if (function_args[0] == kRefineSplitFuncId) {
      if (function_args.size() >= 3 && function_args[1].is_string() &&
          function_args[2].is_int()) {
        result = RefineSplit(result, function_args[1].GetString(),
                             function_args[2].GetInt());
      }
    }
  }
  return result;
}

void MaybeSetQueryInResult(const ScrapeRule& rule,
                           const std::string& value,
                           PageScrapeResult* result) {
  if (rule.rule_type != ScrapeRuleType::kSearchQuery &&
      rule.rule_type != ScrapeRuleType::kWidgetTitle) {
    return;
  }
  result->query = value;
}

}  // namespace

PageScrapeResult::PageScrapeResult(GURL url, std::string id)
    : url(url), id(id) {}
PageScrapeResult::~PageScrapeResult() = default;

ContentScraper::ContentScraper(
    std::unique_ptr<ServerConfig>* last_loaded_server_config,
    std::unique_ptr<PatternsGroup>* patterns)
    : pool_sequenced_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({})),
      last_loaded_server_config_(last_loaded_server_config),
      patterns_(patterns) {}

ContentScraper::~ContentScraper() = default;

base::Value PageScrapeResult::SerializeToValue() {
  base::Value::Dict result;
  base::Value::Dict fields_dict;

  for (const auto& [root_selector, inner_fields] : fields) {
    base::Value::List list;
    for (const auto& values : inner_fields) {
      list.Append(values.Clone());
    }
    fields_dict.Set(root_selector, std::move(list));
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
  for (const auto [root_selector, inner_fields_val] : *fields_dict) {
    const auto* inner_fields_list = inner_fields_val.GetIfList();
    if (!inner_fields_list) {
      continue;
    }
    for (const auto& values : *inner_fields_list) {
      const auto* values_dict = values.GetIfDict();
      if (!values_dict) {
        continue;
      }
      result->fields[root_selector].push_back(values_dict->Clone());
    }
  }

  return result;
}

void ContentScraper::ScrapePage(const GURL& url,
                                bool is_strict_scrape,
                                mojom::DocumentExtractor* document_extractor,
                                PageScrapeResultCallback callback) {
  const auto* url_details =
      (*patterns_)->GetMatchingURLPattern(url, is_strict_scrape);
  if (!url_details) {
    return;
  }
  auto interim_result =
      std::make_unique<PageScrapeResult>(url, url_details->id);

  std::vector<mojom::SelectRequestPtr> select_requests;
  for (const auto& [selector, group] : url_details->scrape_rule_groups) {
    auto select_request = mojom::SelectRequest::New();
    select_request->root_selector = selector;
    for (const auto& [report_key, rule] : group) {
      if (rule->rule_type == ScrapeRuleType::kStandard) {
        ProcessStandardRule(report_key, *rule, selector, url,
                            interim_result.get());
        continue;
      }
      auto attribute_request = mojom::SelectAttributeRequest::New();
      attribute_request->sub_selector = rule->sub_selector;
      attribute_request->attribute = rule->attribute;
      attribute_request->key = report_key;

      select_request->attribute_requests.push_back(
          std::move(attribute_request));
    }
    select_requests.push_back(std::move(select_request));
  }

  document_extractor->QueryElementAttributes(
      std::move(select_requests),
      base::BindOnce(&ContentScraper::OnScrapedElementAttributes,
                     base::Unretained(this), is_strict_scrape,
                     std::move(interim_result), std::move(callback)));
}

void ContentScraper::ParseAndScrapePage(
    bool is_strict_scrape,
    std::unique_ptr<PageScrapeResult> prev_result,
    std::string html,
    PageScrapeResultCallback callback) {
  const auto* url_details =
      (*patterns_)->GetMatchingURLPattern(prev_result->url, is_strict_scrape);
  if (!url_details) {
    return;
  }
  auto interim_result = std::move(prev_result);

  std::vector<rust_document_extractor::SelectRequest> select_requests;
  for (const auto& [selector, group] : url_details->scrape_rule_groups) {
    rust_document_extractor::SelectRequest select_request;
    select_request.root_selector = selector;
    for (const auto& [report_key, rule] : group) {
      if (rule->rule_type == ScrapeRuleType::kStandard) {
        ProcessStandardRule(report_key, *rule, selector, interim_result->url,
                            interim_result.get());
        continue;
      }
      rust_document_extractor::SelectAttributeRequest attribute_request{
          .sub_selector = rule->sub_selector.value_or(""),
          .key = report_key,
          .attribute = rule->attribute,
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
                     weak_ptr_factory_.GetWeakPtr(), is_strict_scrape,
                     std::move(interim_result), std::move(callback)));
}

void ContentScraper::ProcessStandardRule(const std::string& report_key,
                                         const ScrapeRule& rule,
                                         const std::string& root_selector,
                                         const GURL& url,
                                         PageScrapeResult* scrape_result) {
  auto& fields = scrape_result->fields[root_selector];
  if (fields.empty()) {
    fields.emplace_back();
  }
  if (rule.attribute == kUrlAttrId) {
    fields[0].Set(report_key, url.spec());
  } else if (rule.attribute == kCountryCodeAttrId) {
    fields[0].Set(report_key, (*last_loaded_server_config_)->location);
  }
}

void ContentScraper::OnScrapedElementAttributes(
    bool is_strict_scrape,
    std::unique_ptr<PageScrapeResult> scrape_result,
    PageScrapeResultCallback callback,
    std::vector<mojom::AttributeResultPtr> attribute_results) {
  const auto* url_details =
      (*patterns_)->GetMatchingURLPattern(scrape_result->url, is_strict_scrape);
  if (!url_details) {
    return;
  }
  for (const auto& attribute_result : attribute_results) {
    const auto rule_group =
        url_details->scrape_rule_groups.find(attribute_result->root_selector);
    if (rule_group == url_details->scrape_rule_groups.end()) {
      continue;
    }
    base::Value::Dict attribute_values;
    for (const auto& [key, value_str] : attribute_result->attribute_values) {
      const auto rule = rule_group->second.find(key);
      if (rule == rule_group->second.end()) {
        continue;
      }
      base::Value value;
      if (value_str) {
        std::string final_value_str =
            ExecuteRefineFunctions(rule->second->functions_applied, *value_str);
        MaybeSetQueryInResult(*rule->second, final_value_str,
                              scrape_result.get());
        value = base::Value(final_value_str);
      }
      attribute_values.Set(key, std::move(value));
    }
    scrape_result->fields[attribute_result->root_selector].push_back(
        std::move(attribute_values));
  }
  std::move(callback).Run(std::move(scrape_result));
}

void ContentScraper::OnRustElementAttributes(
    bool is_strict_scrape,
    std::unique_ptr<PageScrapeResult> scrape_result,
    PageScrapeResultCallback callback,
    rust::Vec<rust_document_extractor::AttributeResult> attribute_results) {
  const auto* url_details =
      (*patterns_)->GetMatchingURLPattern(scrape_result->url, is_strict_scrape);
  if (!url_details) {
    return;
  }
  for (const auto& attribute_result : attribute_results) {
    const auto root_selector = std::string(attribute_result.root_selector);
    const auto rule_group = url_details->scrape_rule_groups.find(root_selector);
    if (rule_group == url_details->scrape_rule_groups.end()) {
      continue;
    }
    base::Value::Dict attribute_values;
    for (const auto& pair : attribute_result.attribute_pairs) {
      const auto key_str = std::string(pair.key);
      const auto rule = rule_group->second.find(key_str);
      if (rule == rule_group->second.end()) {
        continue;
      }
      base::Value value;
      if (!pair.value.empty()) {
        std::string value_str = ExecuteRefineFunctions(
            rule->second->functions_applied, std::string(pair.value));
        MaybeSetQueryInResult(*rule->second, value_str, scrape_result.get());
        value = base::Value(value_str);
      }
      attribute_values.Set(key_str, std::move(value));
    }
    scrape_result->fields[root_selector].push_back(std::move(attribute_values));
  }
  std::move(callback).Run(std::move(scrape_result));
}

}  // namespace web_discovery
