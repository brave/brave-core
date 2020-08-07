/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/legacy/bat_util.h"
#include "bat/ledger/internal/logging/logging.h"
#include "bat/ledger/internal/legacy/report_balance_state.h"
#include "base/json/json_reader.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kAutoContributionsKey[] = "auto_contribute";
const char kAdEarningsKey[] = "earning_from_ads";
const char kGrantsKey[] = "grants";
const char kOneTimeDonationsKey[] = "one_time_donation";
const char kRecurringDonationsKey[] = "recurring_donation";

bool GetPropertyFromDict(
    const base::DictionaryValue* dictionary,
    const std::string& key,
    double* value) {
  if (!value) {
    return false;
  }

  const auto balance = dictionary->FindDoubleKey(key);
  if (balance) {
    *value = *balance;
    return true;
  }

  const auto* balance_probi = dictionary->FindStringKey(key);
  if (!balance_probi) {
    return false;
  }

  *value = braveledger_bat_util::ProbiToDouble(*balance_probi);
  return true;
}

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
  bool result = GetPropertyFromDict(
      dictionary,
      kGrantsKey,
      &report_balance_properties.grants);

  if (!result) {
    NOTREACHED();
    return false;
  }

  // Earnings From Ads
  result = GetPropertyFromDict(
      dictionary,
      kAdEarningsKey,
      &report_balance_properties.ad_earnings);

  if (!result) {
    NOTREACHED();
    return false;
  }

  // Auto Contribute
  result = GetPropertyFromDict(
      dictionary,
      kAutoContributionsKey,
      &report_balance_properties.auto_contributions);

  if (!result) {
    NOTREACHED();
    return false;
  }

  // Recurring Donation
  result = GetPropertyFromDict(
      dictionary,
      kRecurringDonationsKey,
      &report_balance_properties.recurring_donations);

  if (!result) {
    NOTREACHED();
    return false;
  }

  // One Time Donation
  result = GetPropertyFromDict(
      dictionary,
      kOneTimeDonationsKey,
      &report_balance_properties.one_time_donations);

  if (!result) {
    NOTREACHED();
    return false;
  }

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
  writer->Double(properties.grants);

  writer->String(kAdEarningsKey);
  writer->Double(properties.ad_earnings);

  writer->String(kAutoContributionsKey);
  writer->Double(properties.auto_contributions);

  writer->String(kRecurringDonationsKey);
  writer->Double(properties.recurring_donations);

  writer->String(kOneTimeDonationsKey);
  writer->Double(properties.one_time_donations);

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
