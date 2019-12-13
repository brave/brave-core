/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/report_balance_state.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kAutoContributionsKey[] = "auto_contribute";
const char kAdEarningsKey[] = "earning_from_ads";
const char kGrantsKey[] = "grants";
const char kOneTimeDonationsKey[] = "one_time_donation";
const char kRecurringDonationsKey[] = "recurring_donation";

}  // namespace

ReportBalanceState::ReportBalanceState() = default;

ReportBalanceState::~ReportBalanceState() = default;

bool ReportBalanceState::FromJson(
    const std::string& json,
    ReportBalanceProperties* properties) const {
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

bool ReportBalanceState::FromDict(
    const base::DictionaryValue* dictionary,
    ReportBalanceProperties* properties) const {
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

  ReportBalanceProperties report_balance_properties;

  // Grants
  const auto* grants = dictionary->FindStringKey(kGrantsKey);
  if (!grants) {
    NOTREACHED();
    return false;
  }
  report_balance_properties.grants = *grants;

  // Earnings From Ads
  const auto* ad_earnings = dictionary->FindStringKey(kAdEarningsKey);
  if (!ad_earnings) {
    NOTREACHED();
    return false;
  }
  report_balance_properties.ad_earnings = *ad_earnings;

  // Auto Contribute
  const auto* auto_contributions =
      dictionary->FindStringKey(kAutoContributionsKey);
  if (!auto_contributions) {
    NOTREACHED();
    return false;
  }
  report_balance_properties.auto_contributions = *auto_contributions;

  // Recurring Donation
  const auto* recurring_donations =
      dictionary->FindStringKey(kRecurringDonationsKey);
  if (!recurring_donations) {
    NOTREACHED();
    return false;
  }
  report_balance_properties.recurring_donations = *recurring_donations;

  // One Time Donation
  const auto* one_time_donations =
      dictionary->FindStringKey(kOneTimeDonationsKey);
  if (!one_time_donations) {
    NOTREACHED();
    return false;
  }
  report_balance_properties.one_time_donations = *one_time_donations;

  *properties = report_balance_properties;

  return true;
}

bool ReportBalanceState::ToJson(
    JsonWriter* writer,
    const ReportBalanceProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kGrantsKey);
  writer->String(properties.grants.c_str());

  writer->String(kAdEarningsKey);
  writer->String(properties.ad_earnings.c_str());

  writer->String(kAutoContributionsKey);
  writer->String(properties.auto_contributions.c_str());

  writer->String(kRecurringDonationsKey);
  writer->String(properties.recurring_donations.c_str());

  writer->String(kOneTimeDonationsKey);
  writer->String(properties.one_time_donations.c_str());

  writer->EndObject();

  return true;
}

std::string ReportBalanceState::ToJson(
    const ReportBalanceProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
