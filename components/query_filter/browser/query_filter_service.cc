// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_service.h"

#include <utility>

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

void QueryFilterService::OnComponentReady(const base::FilePath& install_dir) {
  install_dir_ = install_dir;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     install_dir_.AppendASCII(kQueryFilterJsonFile)),
      base::BindOnce(&QueryFilterService::OnRulesJsonLoaded,
                     weak_factory_.GetWeakPtr()));
}

void QueryFilterService::OnRulesJsonLoaded(const std::string& contents) {
  ParseRulesJson(contents);
}

// See query-filter.json file format in the adblock-lists repository.
void QueryFilterService::ParseRulesJson(const std::string& contents) {
  rules_.clear();

  if (contents.empty()) {
    return;
  }

  auto parsed = base::JSONReader::ReadAndReturnValueWithError(
      contents, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed.has_value()) {
    LOG(ERROR) << "query-filter.json parse error: " << parsed.error().message;
    return;
  }

  // Helper function to insert strings from a list into a vector.
  auto rule_inserter = [](const base::ListValue* lv,
                          std::vector<std::string>& output) {
    for (const base::Value& item : *lv) {
      if (item.is_string()) {
        output.push_back(item.GetString());
      }
    }
  };

  const base::ListValue* list = parsed->GetIfList();
  if (!list) {
    LOG(ERROR) << "query-filter.json: root must be an array";
    return;
  }

  for (const base::Value& entry : *list) {
    const base::DictValue* dict = entry.GetIfDict();
    if (!dict) {
      LOG(ERROR) << "query-filter.json: each entry must be an object";
      continue;
    }

    QueryFilterRule rule;
    rule_inserter(dict->FindList("include"), rule.include);
    rule_inserter(dict->FindList("exclude"), rule.exclude);
    rule_inserter(dict->FindList("params"), rule.params);
    rules_.push_back(std::move(rule));
  }
}

}  // namespace query_filter
