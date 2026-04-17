// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_service.h"

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/query_filter/common/features.h"

namespace query_filter {

namespace {

constexpr char kQueryFilterJsonFile[] = "query-filter.json";
constexpr char kQueryFilterRulesIncludeKey[] = "include";
constexpr char kQueryFilterRulesExcludeKey[] = "exclude";
constexpr char kQueryFilterRulesParamsKey[] = "params";

}  // namespace

// QueryFilterRule data structure.
QueryFilterRule::QueryFilterRule() = default;
QueryFilterRule::~QueryFilterRule() = default;
QueryFilterRule::QueryFilterRule(const QueryFilterRule& other) = default;
QueryFilterRule& QueryFilterRule::operator=(const QueryFilterRule& other) =
    default;

// QueryFilterService implementation.
// static
QueryFilterService* QueryFilterService::GetInstance() {
  if (!base::FeatureList::IsEnabled(features::kQueryFilterComponent)) {
    return nullptr;
  }
  return base::Singleton<QueryFilterService>::get();
}

QueryFilterService::QueryFilterService() = default;

QueryFilterService::~QueryFilterService() = default;

void QueryFilterService::OnComponentReady(base::FilePath install_dir) {
  install_dir_ = install_dir;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN, base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     install_dir_.AppendASCII(kQueryFilterJsonFile)),
      base::BindOnce(&QueryFilterService::OnRulesJsonLoaded,
                     weak_factory_.GetWeakPtr()));
}

void QueryFilterService::OnRulesJsonLoaded(const std::string& contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto rules = ParseRulesJson(contents);
  if (!rules.empty()) {
    rules_ = std::move(rules);
  }
}

std::vector<QueryFilterRule> ParseRulesJson(const std::string_view contents) {
  if (contents.empty()) {
    return {};
  }

  auto parsed = base::JSONReader::ReadAndReturnValueWithError(
      contents, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed.has_value()) {
    DLOG(ERROR) << "query-filter.json parse error: " << parsed.error().message;
    return {};
  }

  const base::ListValue* root_list = parsed->GetIfList();
  if (!root_list) {
    DLOG(ERROR) << "query-filter.json: root must be an array";
    return {};
  }

  std::vector<QueryFilterRule> rules;

  // Helper to insert valid strings from a list into a vector.
  auto valid_string_inserter = [](const base::ListValue* lv,
                                  std::vector<std::string>& output) {
    if (!lv || lv->empty()) {
      return;
    }
    const size_t precomputed_output_size =
        std::count_if(lv->begin(), lv->end(),
                      [](const base::Value& item) { return item.is_string(); });
    output.reserve(precomputed_output_size);
    std::for_each(lv->begin(), lv->end(), [&output](const base::Value& item) {
      if (item.is_string()) {
        output.push_back(item.GetString());
      } else {
        DLOG(ERROR) << "query-filter.json: non-string item found in list";
      }
    });
  };

  // Parse each rule entry in the list.
  for (const base::Value& entry : *root_list) {
    const base::DictValue* rule_dict = entry.GetIfDict();
    if (!rule_dict) {
      DLOG(ERROR) << "query-filter.json: non dict rule entry found";
      continue;
    }

    if (!rule_dict->FindList(kQueryFilterRulesIncludeKey) ||
        !rule_dict->FindList(kQueryFilterRulesExcludeKey) ||
        !rule_dict->FindList(kQueryFilterRulesParamsKey)) {
      DLOG(ERROR) << "query-filter.json: missing required fields";
      continue;
    }

    if (rule_dict->size() != 3) {
      DLOG(ERROR) << "query-filter.json: rule entry must have exactly 3 "
                     "fields: include, exclude and params";
      continue;
    }

    QueryFilterRule rule;
    valid_string_inserter(rule_dict->FindList(kQueryFilterRulesIncludeKey),
                          rule.include);
    valid_string_inserter(rule_dict->FindList(kQueryFilterRulesExcludeKey),
                          rule.exclude);
    valid_string_inserter(rule_dict->FindList(kQueryFilterRulesParamsKey),
                          rule.params);
    rules.push_back(std::move(rule));
  }

  return rules;
}

}  // namespace query_filter
