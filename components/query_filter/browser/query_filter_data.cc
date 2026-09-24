// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_data.h"

#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "brave/components/query_filter/common/features.h"

namespace query_filter {

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

const std::string QueryFilterData::GetVersion() const {
  if (!version_.IsValid()) {
    return std::string();
  }
  return version_.GetString();
}

bool QueryFilterData::PopulateDataFromComponent(std::string_view json_data) {
  if (json_data.empty()) {
    LOG(ERROR)
        << "query-filter.json parse error: Nothing to parse, json is empty.";
    return false;
  }

  auto parsed = base::JSONReader::ReadAndReturnValueWithError(
      json_data, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed.has_value() || !parsed->is_list()) {
    std::string error_message = !parsed.has_value()
                                    ? parsed.error().message
                                    : "Root should be base::List type";
    LOG(ERROR) << "query-filter.json parse error: " << error_message;
    return false;
  }

  std::vector<schema::Rule> new_rules;
  for (const base::Value& rule_raw : parsed->GetList()) {
    auto rule = schema::Rule::FromValue(rule_raw);
    if (!rule.has_value()) {
      LOG(ERROR) << "query-filter.json parse error: " << rule.error();
    } else {
      new_rules.emplace_back(std::move(rule.value()));
    }
  }

  // No new valid rules found.
  if (new_rules.empty()) {
    LOG(ERROR) << "query-filter.json parse error: No valid rules found.";
    return false;
  }

  rules_ = std::move(new_rules);
  return true;
}

void QueryFilterData::ResetRulesForTesting() noexcept {
  rules_.clear();
  version_ = base::Version();
}

}  // namespace query_filter
