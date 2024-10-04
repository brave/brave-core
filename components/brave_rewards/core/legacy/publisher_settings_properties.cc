/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/legacy/publisher_settings_properties.h"

#include <utility>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "brave/components/brave_rewards/core/constants.h"

namespace brave_rewards::internal {

namespace {

// Do not change these values as they are required to transition legacy state
constexpr char kAllowNonVerifiedSitesInListKey[] = "allow_non_verified";
// There is a spelling error with min_pubslisher_duration, however we cannot
// change this otherwise we will break legacy installs. This will be resolved as
// part of https://github.com/brave/brave-browser/issues/7024
constexpr char kMinPageTimeBeforeLoggingAVisitKey[] = "min_pubslisher_duration";
constexpr char kMinVisitsForPublisherRelevancy[] = "min_visits";
constexpr char kMonthlyBalancesKey[] = "monthly_balances";
constexpr char kProcessedPendingPublishersKey[] =
    "processed_pending_publishers";

}  // namespace

PublisherSettingsProperties::PublisherSettingsProperties()
    : min_page_time_before_logging_a_visit(8),
      min_visits_for_publisher_relevancy(1),
      allow_non_verified_sites_in_list(true) {}

PublisherSettingsProperties::PublisherSettingsProperties(
    PublisherSettingsProperties&& other) = default;
PublisherSettingsProperties& PublisherSettingsProperties::operator=(
    PublisherSettingsProperties&& other) = default;

PublisherSettingsProperties::~PublisherSettingsProperties() = default;

bool PublisherSettingsProperties::operator==(
    const PublisherSettingsProperties& rhs) const {
  return min_page_time_before_logging_a_visit ==
             rhs.min_page_time_before_logging_a_visit &&
         min_visits_for_publisher_relevancy ==
             rhs.min_visits_for_publisher_relevancy &&
         allow_non_verified_sites_in_list ==
             rhs.allow_non_verified_sites_in_list &&
         monthly_balances == rhs.monthly_balances &&
         processed_pending_publishers == rhs.processed_pending_publishers;
}

bool PublisherSettingsProperties::operator!=(
    const PublisherSettingsProperties& rhs) const {
  return !(*this == rhs);
}

base::Value::Dict PublisherSettingsProperties::ToValue() const {
  base::Value::Dict dict;

  dict.Set(kMinPageTimeBeforeLoggingAVisitKey,
           base::Int64ToValue(min_page_time_before_logging_a_visit));
  dict.Set(kMinVisitsForPublisherRelevancy,
           base::Int64ToValue(min_visits_for_publisher_relevancy));
  dict.Set(kAllowNonVerifiedSitesInListKey, allow_non_verified_sites_in_list);

  base::Value::List monthly_balances_list;
  for (const auto& [key, value] : monthly_balances) {
    base::Value::Dict item;
    item.Set(key, value.ToValue());
    monthly_balances_list.Append(std::move(item));
  }
  dict.Set(kMonthlyBalancesKey, std::move(monthly_balances_list));

  base::Value::List prcessed_pending_publishers_list;
  for (const auto& processed_pending_publisher : processed_pending_publishers) {
    prcessed_pending_publishers_list.Append(processed_pending_publisher);
  }
  dict.Set(kProcessedPendingPublishersKey,
           std::move(prcessed_pending_publishers_list));

  return dict;
}

bool PublisherSettingsProperties::FromValue(const base::Value::Dict& dict) {
  // Min Publisher Duration (There is no support for uint64_t. Writing JSON with
  // such types violates the spec. As we need a uint64_t, we need to use a
  // double and cast to a uint64_t)
  if (auto min_page_time_value_int64 =
          base::ValueToInt64(dict.Find(kMinPageTimeBeforeLoggingAVisitKey))) {
    min_page_time_before_logging_a_visit = *min_page_time_value_int64;
  } else if (auto min_page_time_value_double =
                 dict.FindDouble(kMinPageTimeBeforeLoggingAVisitKey)) {
    min_page_time_before_logging_a_visit =
        static_cast<uint64_t>(*min_page_time_value_double);
  } else {
    NOTREACHED_IN_MIGRATION();
    return false;
  }

  // Min Visits (There is no support for unsigned int. Writing JSON with such
  // types violates the spec. As we need an unsigned int, we need to a double
  // and cast to an unsigned int)
  if (auto min_visits_value_int64 =
          base::ValueToInt64(dict.Find(kMinVisitsForPublisherRelevancy))) {
    min_visits_for_publisher_relevancy = *min_visits_value_int64;
  } else if (auto min_visits_value_double =
                 dict.FindDouble(kMinVisitsForPublisherRelevancy)) {
    min_visits_for_publisher_relevancy =
        static_cast<unsigned int>(*min_visits_value_double);
  } else {
    NOTREACHED_IN_MIGRATION();
    return false;
  }

  // Allow Non Verified
  if (auto value = dict.FindBool(kAllowNonVerifiedSitesInListKey)) {
    allow_non_verified_sites_in_list = *value;
  } else {
    NOTREACHED_IN_MIGRATION();
    return false;
  }

  // Monthly Balances
  const auto* monthly_balances_list = dict.FindList(kMonthlyBalancesKey);
  if (!monthly_balances_list) {
    NOTREACHED_IN_MIGRATION();
    return false;
  }

  for (const auto& item : *monthly_balances_list) {
    const auto* monthly_balance_value = item.GetIfDict();
    if (!monthly_balance_value) {
      NOTREACHED_IN_MIGRATION();
      continue;
    }

    for (const auto [key, value] : *monthly_balance_value) {
      if (!value.is_dict()) {
        NOTREACHED_IN_MIGRATION();
        continue;
      }
      ReportBalanceProperties report_balance;
      if (!report_balance.FromValue(value.GetDict())) {
        continue;
      }

      monthly_balances.emplace(key, report_balance);
    }
  }

  // Processed Pending Publishers
  if (const auto* value = dict.FindList(kProcessedPendingPublishersKey)) {
    for (const auto& processed_pending_publisher_value : *value) {
      if (!processed_pending_publisher_value.is_string()) {
        NOTREACHED_IN_MIGRATION();
        continue;
      }

      processed_pending_publishers.push_back(
          processed_pending_publisher_value.GetString());
    }
  }

  return true;
}

std::string PublisherSettingsProperties::ToJson() const {
  std::string json;
  CHECK(base::JSONWriter::Write(ToValue(), &json));
  return json;
}

bool PublisherSettingsProperties::FromJson(const std::string& json) {
  auto document = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);

  if (!document.has_value()) {
    LOG(ERROR) << "Invalid client property. json=" << json
               << ", error line=" << document.error().line
               << ", error column=" << document.error().column
               << ", error message=" << document.error().message;
    return false;
  }

  const base::Value::Dict* root = document->GetIfDict();
  if (!root) {
    LOG(ERROR) << "Invalid client property. json=" << json;
    return false;
  }

  return FromValue(*root);
}

}  // namespace brave_rewards::internal
