/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/content_scraper.h"

#include <algorithm>
#include <utility>

#include "base/containers/map_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_split.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/web_discovery/browser/document_extractor/lib.rs.h"
#include "brave/components/web_discovery/browser/legacy_refine_functions.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "brave/components/web_discovery/browser/patterns_v2.h"
#include "brave/components/web_discovery/browser/privacy_guard.h"
#include "brave/components/web_discovery/browser/relevant_site.h"
#include "brave/components/web_discovery/browser/url_extractor.h"
#include "brave/components/web_discovery/browser/util.h"
#include "brave/components/web_discovery/browser/value_transform.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace web_discovery {

namespace {

constexpr char kFieldsValueKey[] = "fields";
constexpr char kIdValueKey[] = "id";
constexpr char kUrlValueKey[] = "url";

class ContentScraperImpl : public ContentScraper {
 public:
  explicit ContentScraperImpl(const ServerConfigLoader* server_config_loader,
                              const URLExtractor* url_extractor)
      : sequenced_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
            {base::TaskPriority::BEST_EFFORT,
             base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
        server_config_loader_(server_config_loader),
        url_extractor_(url_extractor) {}

  ~ContentScraperImpl() override = default;

  ContentScraperImpl(const ContentScraperImpl&) = delete;
  ContentScraperImpl& operator=(const ContentScraperImpl&) = delete;

  void ScrapePage(const GURL& url,
                  bool is_strict_scrape,
                  mojom::DocumentExtractor* document_extractor,
                  PageScrapeResultCallback callback) override {
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
        base::BindOnce(&ContentScraperImpl::OnRendererScrapedElementAttributes,
                       base::Unretained(this), is_strict_scrape,
                       std::move(interim_result), std::move(callback)));
  }

  void ParseAndScrapePage(const GURL& url,
                          bool is_strict_scrape,
                          std::unique_ptr<PageScrapeResult> prev_result,
                          std::string html,
                          PageScrapeResultCallback callback) override {
    const auto* url_details =
        server_config_loader_->GetLastPatterns().GetMatchingURLPattern(
            url, is_strict_scrape);
    if (!url_details) {
      return;
    }
    auto interim_result = std::move(prev_result);

    std::vector<SelectRequest> select_requests;
    for (const auto& [selector, group] : url_details->scrape_rule_groups) {
      SelectRequest select_request;
      select_request.root_selector = selector;
      for (const auto& [report_key, rule] : group) {
        if (rule->rule_type == ScrapeRuleType::kStandard) {
          ProcessStandardRule(report_key, *rule, selector, url,
                              interim_result.get());
          continue;
        }
        SelectAttributeRequest attribute_request{
            .key = report_key,
            .options = {SelectAttributeOption{
                .sub_selector = rule->sub_selector.value_or(""),
                .attribute = rule->attribute,
            }}};

        select_request.attribute_requests.push_back(
            std::move(attribute_request));
      }
      select_requests.push_back(std::move(select_request));
    }

    sequenced_task_runner_->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(&query_element_attributes, html, select_requests),
        base::BindOnce(&ContentScraperImpl::OnBrowserParsedElementAttributes,
                       weak_ptr_factory_.GetWeakPtr(), is_strict_scrape,
                       std::move(interim_result), std::move(callback)));
  }

  void ParseAndScrapePageV2(const GURL& url,
                            std::string response_body,
                            PageScrapeResultCallback callback) override {
    // Get the v2 patterns from server config loader
    const auto& v2_patterns = server_config_loader_->GetLastV2Patterns();

    // Use URLExtractor to identify the relevant site for this URL
    auto url_result = url_extractor_->IdentifyURL(url);
    if (!url_result) {
      std::move(callback).Run(nullptr);
      return;
    }

    const RelevantSite site = url_result->details->site;
    const auto* site_pattern =
        base::FindOrNull(v2_patterns.site_patterns, site);
    if (!site_pattern) {
      std::move(callback).Run(nullptr);
      return;
    }

    // Get the site ID for the PageScrapeResult
    auto site_id = RelevantSiteToID(site);
    CHECK(site_id);
    auto interim_result =
        std::make_unique<PageScrapeResult>(url, std::string(*site_id));
    interim_result->query = url_result->query;

    // Convert v2 input groups to SelectRequest format
    std::vector<SelectRequest> select_requests;
    for (const auto& [selector, input_group] : site_pattern->input_groups) {
      SelectRequest select_request;
      select_request.root_selector = selector;
      select_request.select_all = input_group.select_all;

      // Convert extraction rules to attribute requests
      for (const auto& [key, extraction_rules] : input_group.extraction_rules) {
        SelectAttributeRequest attribute_request{
            .key = key,
        };

        // Convert each extraction rule to a SelectAttributeOption
        for (const auto& extraction_rule : extraction_rules) {
          SelectAttributeOption option{
              .sub_selector = extraction_rule.sub_selector.value_or(""),
              .attribute = extraction_rule.attribute,
          };
          attribute_request.options.push_back(std::move(option));
        }

        select_request.attribute_requests.push_back(
            std::move(attribute_request));
      }
      select_requests.push_back(std::move(select_request));
    }

    // Use browser-based extraction (similar to ParseAndScrapePage)
    sequenced_task_runner_->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(&query_element_attributes, response_body,
                       select_requests),
        base::BindOnce(&ContentScraperImpl::OnBrowserParsedV2ElementAttributes,
                       weak_ptr_factory_.GetWeakPtr(), site,
                       std::move(interim_result), std::move(callback)));
  }

 private:
  void ProcessStandardRule(const std::string& report_key,
                           const ScrapeRule& rule,
                           const std::string& root_selector,
                           const GURL& url,
                           PageScrapeResult* scrape_result) {
    auto value = GetRequestValue(rule.attribute, url,
                                 server_config_loader_->GetLastServerConfig(),
                                 *scrape_result);
    if (!value) {
      return;
    }
    auto refined_value = ExecuteRefineFunctions(rule.functions_applied, *value);
    if (!refined_value) {
      return;
    }
    auto& fields = scrape_result->fields[root_selector];
    if (fields.empty()) {
      fields.emplace_back();
    }
    fields[0].Set(report_key, *refined_value);
  }

  void OnRendererScrapedElementAttributes(
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
        ProcessAttributeValue(rule_group->second, *scrape_result, key,
                              value_str, attribute_values);
      }
      scrape_result->fields[attribute_result->root_selector].push_back(
          std::move(attribute_values));
    }
    std::move(callback).Run(std::move(scrape_result));
  }

  void OnBrowserParsedElementAttributes(
      bool is_strict_scrape,
      std::unique_ptr<PageScrapeResult> scrape_result,
      PageScrapeResultCallback callback,
      rust::Vec<AttributeResult> attribute_results) {
    const auto* url_details =
        server_config_loader_->GetLastPatterns().GetMatchingURLPattern(
            scrape_result->url, is_strict_scrape);
    if (!url_details) {
      return;
    }
    for (const auto& attribute_result : attribute_results) {
      const auto root_selector = std::string(attribute_result.root_selector);
      const auto rule_group =
          url_details->scrape_rule_groups.find(root_selector);
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
      scrape_result->fields[root_selector].push_back(
          std::move(attribute_values));
    }
    std::move(callback).Run(std::move(scrape_result));
  }

  void OnBrowserParsedV2ElementAttributes(
      const RelevantSite& site,
      std::unique_ptr<PageScrapeResult> scrape_result,
      PageScrapeResultCallback callback,
      rust::Vec<AttributeResult> attribute_results) {
    // Get the v2 patterns to process results
    const auto& v2_patterns = server_config_loader_->GetLastV2Patterns();
    const auto* site_pattern =
        base::FindOrNull(v2_patterns.site_patterns, site);
    if (!site_pattern) {
      std::move(callback).Run(nullptr);
      return;
    }

    // Process extracted attributes according to v2 pattern rules
    for (const auto& attribute_result : attribute_results) {
      // Find the corresponding input group using the selector as key
      const auto root_selector = std::string(attribute_result.root_selector);
      const auto* input_group =
          base::FindOrNull(site_pattern->input_groups, root_selector);
      if (!input_group) {
        continue;
      }

      // Process each attribute pair
      base::Value::Dict attribute_values;
      for (const auto& pair : attribute_result.attribute_pairs) {
        std::string key(pair.key);
        base::Value value;

        // Apply transforms if defined in the extraction rules
        const auto* extraction_rules =
            base::FindOrNull(input_group->extraction_rules, key);
        if (!extraction_rules) {
          continue;
        }
        if (!pair.value.empty()) {
          // Use the option_index to get the specific rule that was used
          size_t rule_index = static_cast<size_t>(pair.option_index);
          if (rule_index >= extraction_rules->size()) {
            continue;
          }
          const auto& extraction_rule = (*extraction_rules)[rule_index];
          if (!extraction_rule.transforms.empty()) {
            auto transformed_value = ApplyTransforms(extraction_rule.transforms,
                                                     std::string(pair.value));
            if (transformed_value) {
              value = base::Value(*transformed_value);
            }
          } else {
            value = base::Value(std::string(pair.value));
          }
        }

        attribute_values.Set(key, std::move(value));
      }

      if (!attribute_values.empty()) {
        scrape_result->fields[root_selector].push_back(
            std::move(attribute_values));
      }
    }

    std::move(callback).Run(std::move(scrape_result));
  }

  void ProcessAttributeValue(const ScrapeRuleGroup& rule_group,
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

  scoped_refptr<base::SequencedTaskRunner> sequenced_task_runner_;

  raw_ptr<const ServerConfigLoader> server_config_loader_;
  raw_ptr<const URLExtractor> url_extractor_;

  base::WeakPtrFactory<ContentScraperImpl> weak_ptr_factory_{this};
};

}  // namespace

std::unique_ptr<ContentScraper> ContentScraper::Create(
    const ServerConfigLoader* server_config_loader,
    const URLExtractor* url_extractor) {
  return std::make_unique<ContentScraperImpl>(server_config_loader,
                                              url_extractor);
}

PageScrapeResult::PageScrapeResult(GURL url, std::string id)
    : url(url), id(id) {}
PageScrapeResult::~PageScrapeResult() = default;

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


}  // namespace web_discovery
