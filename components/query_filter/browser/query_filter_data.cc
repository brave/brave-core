// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_data.h"

#include <algorithm>

#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "brave/components/query_filter/common/features.h"

namespace {
constexpr char kQueryFilterRulesIncludeKey[] = "include";
constexpr char kQueryFilterRulesExcludeKey[] = "exclude";
constexpr char kQueryFilterRulesParamsKey[] = "params";
}  // namespace

namespace query_filter {

// QueryFilterRule data structure.
QueryFilterRule::QueryFilterRule() = default;
QueryFilterRule::~QueryFilterRule() = default;
QueryFilterRule::QueryFilterRule(const QueryFilterRule& other) = default;
QueryFilterRule& QueryFilterRule::operator=(const QueryFilterRule& other) =
    default;

// QueryFilterData implementation.
// static
QueryFilterData* QueryFilterData::GetInstance() {
  if (!base::FeatureList::IsEnabled(features::kQueryFilterComponent)) {
    return nullptr;
  }
  static base::NoDestructor<QueryFilterData> instance;
  return instance.get();
}

QueryFilterData::QueryFilterData() = default;
QueryFilterData::~QueryFilterData() = default;

void QueryFilterData::UpdateVersion(base::Version version) {
  version_ = std::move(version);
}

const std::vector<QueryFilterRule>& QueryFilterData::rules() const {
  return rules_;
}

const std::string QueryFilterData::GetVersion() const {
  if (!version_.IsValid()) {
    return std::string();
  }
  return version_.GetString();
}

// TODO(https://github.com/brave/brave-browser/issues/54775): Update this to use
// an IDL and JSON Compiler to parse the input.
bool QueryFilterData::PopulateDataFromComponent(std::string_view json_data) {
  if (json_data.empty()) {
    return false;
  }
  auto parsed = base::JSONReader::ReadAndReturnValueWithError(
      json_data, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed.has_value()) {
    LOG(WARNING) << "query-filter.json parse error: " << parsed.error().message;
    return false;
  }

  const base::ListValue* root_list = parsed->GetIfList();
  if (!root_list) {
    LOG(WARNING) << "query-filter.json: root must be an array";
    return false;
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
        LOG(WARNING) << "query-filter.json: non-string item found in list";
      }
    });
  };

  // Parse each rule entry in the list.
  for (const base::Value& entry : *root_list) {
    const base::DictValue* rule_dict = entry.GetIfDict();
    if (!rule_dict) {
      LOG(WARNING) << "query-filter.json: non dict rule entry found";
      continue;
    }

    if (!rule_dict->FindList(kQueryFilterRulesIncludeKey) ||
        !rule_dict->FindList(kQueryFilterRulesExcludeKey) ||
        !rule_dict->FindList(kQueryFilterRulesParamsKey)) {
      LOG(WARNING) << "query-filter.json: missing required fields";
      continue;
    }

    if (rule_dict->size() != 3) {
      LOG(WARNING) << "query-filter.json: rule entry must have exactly 3 "
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

  rules_ = std::move(rules);
  return true;
}

void QueryFilterData::ResetRulesForTesting() {
  rules_.clear();
  version_ = base::Version();
}

}  // namespace query_filter
