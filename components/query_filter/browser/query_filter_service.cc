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

QueryFilterRule::QueryFilterRule() = default;
QueryFilterRule::~QueryFilterRule() = default;
QueryFilterRule::QueryFilterRule(const QueryFilterRule& other) = default;
QueryFilterRule& QueryFilterRule::operator=(const QueryFilterRule& other) =
    default;

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

void QueryFilterService::ParseRulesJson(const std::string& contents) {
  rules_.clear();
  is_ready_ = false;

  if (contents.empty()) {
    return;
  }

  auto parsed = base::JSONReader::ReadAndReturnValueWithError(
      contents, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed.has_value()) {
    LOG(ERROR) << "query-filter.json parse error: " << parsed.error().message;
    return;
  }

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
    if (const base::ListValue* lv = dict->FindList("include")) {
      for (const base::Value& item : *lv) {
        if (item.is_string()) {
          rule.include.push_back(item.GetString());
        }
      }
    }
    if (const base::ListValue* lv = dict->FindList("exclude")) {
      for (const base::Value& item : *lv) {
        if (item.is_string()) {
          rule.exclude.push_back(item.GetString());
        }
      }
    }
    if (const base::ListValue* lv = dict->FindList("params")) {
      for (const base::Value& item : *lv) {
        if (item.is_string()) {
          rule.params.push_back(item.GetString());
        }
      }
    }
    rules_.push_back(std::move(rule));
  }

  is_ready_ = true;
}

void QueryFilterService::SetRulesJsonForTesting(const std::string& json) {
  ParseRulesJson(json);
}

}  // namespace query_filter
