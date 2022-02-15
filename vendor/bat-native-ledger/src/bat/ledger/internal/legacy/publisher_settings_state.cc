/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>

#include "base/notreached.h"
#include "bat/ledger/internal/legacy/publisher_settings_state.h"
#include "bat/ledger/internal/legacy/report_balance_state.h"
#include "base/json/json_reader.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kAllowNonVerifiedSitesInListKey[] = "allow_non_verified";
const char kAllowContributionToVideosKey[] = "allow_videos";
// There is a spelling error with min_pubslisher_duration, however we cannot
// change this otherwise we will break legacy installs. This will be resolved as
// part of https://github.com/brave/brave-browser/issues/7024
const char kMinPageTimeBeforeLoggingAVisitKey[] = "min_pubslisher_duration";
const char kMinVisitsForPublisherRelevancy[] = "min_visits";
const char kMonthlyBalancesKey[] = "monthly_balances";
const char kProcessedPendingPublishersKey[] = "processed_pending_publishers";

}  // namespace

PublisherSettingsState::PublisherSettingsState() = default;

PublisherSettingsState::~PublisherSettingsState() = default;

bool PublisherSettingsState::FromJson(
    const std::string& json,
    PublisherSettingsProperties* properties) const {
  DCHECK(properties);
  if (!properties) {
    NOTREACHED();
    return false;
  }

  auto json_value = base::JSONReader::Read(json);
  if (!json_value) {
    NOTREACHED();
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  json_value->GetAsDictionary(&dictionary);
  if (!dictionary) {
    NOTREACHED();
    return false;
  }

  return FromDict(dictionary, properties);
}

bool PublisherSettingsState::FromDict(
    const base::DictionaryValue* dictionary,
    PublisherSettingsProperties* properties) const {
  DCHECK(dictionary);
  if (!dictionary) {
    NOTREACHED();
    return false;
  }

  DCHECK(properties);
  if (!properties) {
    NOTREACHED();
    return false;
  }

  PublisherSettingsProperties publisher_settings_properties;

  // Min Publisher Duration (There is no support for uint64_t. Writing JSON with
  // such types violates the spec. As we need a uint64_t, we need to use a
  // double and cast to a uint64_t)
  const auto min_page_time_before_logging_a_visit =
      dictionary->FindDoubleKey(kMinPageTimeBeforeLoggingAVisitKey);
  if (!min_page_time_before_logging_a_visit) {
    NOTREACHED();
    return false;
  }
  publisher_settings_properties.min_page_time_before_logging_a_visit =
      static_cast<uint64_t>(*min_page_time_before_logging_a_visit);

  // Min Visits (There is no support for unsigned int. Writing JSON with such
  // types violates the spec. As we need an unsigned int, we need to a double
  // and cast to an unsigned int)
  const auto min_visits_for_publisher_relevancy =
      dictionary->FindDoubleKey(kMinVisitsForPublisherRelevancy);
  if (!min_visits_for_publisher_relevancy) {
    NOTREACHED();
    return false;
  }
  publisher_settings_properties.min_visits_for_publisher_relevancy =
      static_cast<unsigned int>(*min_visits_for_publisher_relevancy);

  // Allow Non Verified
  const auto allow_non_verified_sites_in_list =
      dictionary->FindBoolKey(kAllowNonVerifiedSitesInListKey);
  if (!allow_non_verified_sites_in_list) {
    NOTREACHED();
    return false;
  }
  publisher_settings_properties.allow_non_verified_sites_in_list =
      *allow_non_verified_sites_in_list;

  // Allow Videos
  const auto allow_contribution_to_videos =
      dictionary->FindBoolKey(kAllowContributionToVideosKey);
  if (!allow_contribution_to_videos) {
    NOTREACHED();
    return false;
  }
  publisher_settings_properties.allow_contribution_to_videos =
      *allow_contribution_to_videos;

  // Monthly Balances
  const auto* monthly_balances_list =
      dictionary->FindListKey(kMonthlyBalancesKey);
  if (!monthly_balances_list) {
    NOTREACHED();
    return false;
  }

  const ReportBalanceState report_balance_state;
  for (const auto& monthly_balance_value :
       monthly_balances_list->GetListDeprecated()) {
    if (!monthly_balance_value.is_dict()) {
      NOTREACHED();
      continue;
    }

    for (const auto item : monthly_balance_value.DictItems()) {
      const auto& key = item.first;
      const auto& value = item.second;

      if (!value.is_dict()) {
        NOTREACHED();
        continue;
      }

      const base::DictionaryValue* monthly_balance_dictionary = nullptr;
      value.GetAsDictionary(&monthly_balance_dictionary);
      if (!monthly_balance_dictionary) {
        NOTREACHED();
        continue;
      }

      ReportBalanceProperties report_balance;
      if (!report_balance_state.FromDict(monthly_balance_dictionary,
          &report_balance)) {
        continue;
      }

      publisher_settings_properties.monthly_balances.insert(
          {key, report_balance});
    }
  }

  // Processed Pending Publishers
  const auto* processed_pending_publishers_list =
      dictionary->FindListKey(kProcessedPendingPublishersKey);
  if (processed_pending_publishers_list) {
    for (const auto& processed_pending_publisher_value :
         processed_pending_publishers_list->GetListDeprecated()) {
      if (!processed_pending_publisher_value.is_string()) {
        NOTREACHED();
        continue;
      }

      const std::string processed_pending_publisher =
          processed_pending_publisher_value.GetString();

      publisher_settings_properties.processed_pending_publishers.push_back(
          processed_pending_publisher);
    }
  }

  *properties = publisher_settings_properties;

  return true;
}

bool PublisherSettingsState::ToJson(
    JsonWriter* writer,
    const PublisherSettingsProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kMinPageTimeBeforeLoggingAVisitKey);
  writer->Uint(properties.min_page_time_before_logging_a_visit);

  writer->String(kMinVisitsForPublisherRelevancy);
  writer->Uint(properties.min_visits_for_publisher_relevancy);

  writer->String(kAllowNonVerifiedSitesInListKey);
  writer->Bool(properties.allow_non_verified_sites_in_list);

  writer->String(kAllowContributionToVideosKey);
  writer->Bool(properties.allow_contribution_to_videos);

  writer->String(kMonthlyBalancesKey);
  writer->StartArray();
  const ReportBalanceState report_balance_state;
  for (const auto& monthly_balance : properties.monthly_balances) {
    writer->StartObject();
    writer->String(monthly_balance.first.c_str());
    report_balance_state.ToJson(writer, monthly_balance.second);
    writer->EndObject();
  }
  writer->EndArray();

  writer->String(kProcessedPendingPublishersKey);
  writer->StartArray();
  for (const auto &processed_pending_publisher :
      properties.processed_pending_publishers) {
    writer->String(processed_pending_publisher.c_str());
  }
  writer->EndArray();

  writer->EndObject();

  return true;
}

std::string PublisherSettingsState::ToJson(
    const PublisherSettingsProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
