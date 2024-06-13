/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/content_scraper.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "brave/components/web_discovery/browser/privacy_guard.h"
#include "brave/components/web_discovery/browser/util.h"

namespace web_discovery {

namespace {

constexpr char kUrlAttrId[] = "url";
constexpr char kCountryCodeAttrId[] = "ctry";
constexpr char kFieldsValueKey[] = "fields";
constexpr char kIdValueKey[] = "id";
constexpr char kUrlValueKey[] = "url";

constexpr char kRefineSplitFuncId[] = "splitF";
constexpr char kRefineMaskURLFuncId[] = "maskU";
constexpr char kRefineParseURLFuncId[] = "parseU";
constexpr char kRefineJsonExtractFuncId[] = "json";

constexpr char kParseURLQueryExtractType[] = "qs";

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
  return DecodeURLComponent(encoded_result);
}

std::optional<std::string> RefineParseURL(const std::string& value,
                                          const std::string& extract_type,
                                          const std::string& key) {
  if (extract_type != kParseURLQueryExtractType) {
    return std::nullopt;
  }
  GURL url(value);
  if (!url.is_valid() || !url.has_query()) {
    return std::nullopt;
  }
  auto query_value = ExtractValueFromQueryString(url.query_piece(), key);
  return query_value;
}

std::optional<std::string> RefineJsonExtract(const std::string& value,
                                             const std::string& path,
                                             bool extract_objects) {
  auto parsed = base::JSONReader::Read(value);
  if (!parsed || !parsed->is_dict()) {
    return std::nullopt;
  }
  const auto* found_value = parsed->GetDict().FindByDottedPath(path);
  if (!found_value) {
    return std::nullopt;
  }
  if (found_value->is_string()) {
    return found_value->GetString();
  }
  if ((found_value->is_dict() || found_value->is_list()) && !extract_objects) {
    return std::nullopt;
  }
  std::string encoded_value;
  if (!base::JSONWriter::Write(*found_value, &encoded_value)) {
    return std::nullopt;
  }
  return encoded_value;
}

}  // namespace

PageScrapeResult::PageScrapeResult(GURL url, std::string id)
    : url(url), id(id) {}
PageScrapeResult::~PageScrapeResult() = default;

ContentScraper::ContentScraper(const ServerConfigLoader* server_config_loader,
                               RegexUtil* regex_util)
    : pool_sequenced_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({})),
      server_config_loader_(server_config_loader),
      regex_util_(regex_util) {}

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
      server_config_loader_->GetLastPatterns().GetMatchingURLPattern(
          url, is_strict_scrape);
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
    const GURL& url,
    bool is_strict_scrape,
    std::unique_ptr<PageScrapeResult> prev_result,
    std::string html,
    PageScrapeResultCallback callback) {
  const auto* url_details =
      server_config_loader_->GetLastPatterns().GetMatchingURLPattern(
          url, is_strict_scrape);
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
        ProcessStandardRule(report_key, *rule, selector, url,
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
  std::string value;
  if (rule.attribute == kUrlAttrId) {
    value = url.spec();
  } else if (rule.attribute == kCountryCodeAttrId) {
    value = server_config_loader_->GetLastServerConfig().location;
  }
  auto refined_value = ExecuteRefineFunctions(rule.functions_applied, value);
  if (!refined_value) {
    return;
  }
  auto& fields = scrape_result->fields[root_selector];
  if (fields.empty()) {
    fields.emplace_back();
  }
  fields[0].Set(report_key, *refined_value);
}

void ContentScraper::OnScrapedElementAttributes(
    bool is_strict_scrape,
    std::unique_ptr<PageScrapeResult> scrape_result,
    PageScrapeResultCallback callback,
    std::vector<mojom::AttributeResultPtr> attribute_results) {
  const auto* url_details =
      server_config_loader_->GetLastPatterns().GetMatchingURLPattern(
          scrape_result->url, is_strict_scrape);
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
      ProcessAttributeValue(rule_group->second, *scrape_result, key, value_str,
                            attribute_values);
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
      server_config_loader_->GetLastPatterns().GetMatchingURLPattern(
          scrape_result->url, is_strict_scrape);
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
      ProcessAttributeValue(
          rule_group->second, *scrape_result, std::string(pair.key),
          pair.value.empty() ? std::nullopt
                             : std::make_optional<std::string>(pair.value),
          attribute_values);
    }
    scrape_result->fields[root_selector].push_back(std::move(attribute_values));
  }
  std::move(callback).Run(std::move(scrape_result));
}

std::optional<std::string> ContentScraper::ExecuteRefineFunctions(
    const RefineFunctionList& function_list,
    std::string value) {
  std::optional<std::string> result = value;
  for (const auto& function_args : function_list) {
    if (function_args.empty()) {
      continue;
    }
    const auto& func_name = function_args[0];
    if (func_name == kRefineSplitFuncId) {
      if (function_args.size() >= 3 && function_args[1].is_string() &&
          function_args[2].is_int()) {
        result = RefineSplit(*result, function_args[1].GetString(),
                             function_args[2].GetInt());
      }
    } else if (func_name == kRefineMaskURLFuncId) {
      result = MaskURL(*regex_util_, GURL(value));
    } else if (func_name == kRefineParseURLFuncId) {
      if (function_args.size() >= 3 && function_args[1].is_string() &&
          function_args[2].is_string()) {
        result = RefineParseURL(*result, function_args[1].GetString(),
                                function_args[2].GetString());
      }
    } else if (func_name == kRefineJsonExtractFuncId) {
      if (function_args.size() >= 3 && function_args[1].is_string() &&
          function_args[2].is_bool()) {
        result = RefineJsonExtract(*result, function_args[1].GetString(),
                                   function_args[2].GetBool());
      }
    }
    if (!result) {
      break;
    }
  }
  return result;
}

void ContentScraper::ProcessAttributeValue(
    const ScrapeRuleGroup& rule_group,
    PageScrapeResult& scrape_result,
    std::string key,
    std::optional<std::string> value_str,
    base::Value::Dict& attribute_values) {
  const auto rule = rule_group.find(key);
  if (rule == rule_group.end()) {
    return;
  }
  base::Value value;
  if (value_str) {
    value_str =
        ExecuteRefineFunctions(rule->second->functions_applied, *value_str);

    if (value_str) {
      if (rule->second->rule_type == ScrapeRuleType::kSearchQuery ||
          rule->second->rule_type == ScrapeRuleType::kWidgetTitle) {
        scrape_result.query = value_str;
      }

      value = base::Value(*value_str);
    }
  }
  attribute_values.Set(key, std::move(value));
}

}  // namespace web_discovery
