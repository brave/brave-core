/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/legacy/report_balance_properties.h"

#include <string_view>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "brave/components/brave_rewards/core/legacy/bat_util.h"

namespace brave_rewards::internal {

namespace {

// Do not change these values as they are required to transition legacy state
const char kAutoContributionsKey[] = "auto_contribute";
const char kAdEarningsKey[] = "earning_from_ads";
const char kGrantsKey[] = "grants";
const char kOneTimeDonationsKey[] = "one_time_donation";
const char kRecurringDonationsKey[] = "recurring_donation";

bool GetPropertyFromDict(const base::Value::Dict& dict,
                         std::string_view key,
                         double* value) {
  DCHECK(value);

  const auto balance = dict.FindDouble(key);
  if (balance) {
    *value = *balance;
    return true;
  }

  const auto* balance_probi = dict.FindString(key);
  if (!balance_probi) {
    return false;
  }

  *value = ProbiToDouble(*balance_probi);
  return true;
}

}  // namespace

ReportBalanceProperties::ReportBalanceProperties()
    : grants(0.0),
      ad_earnings(0.0),
      auto_contributions(0.0),
      recurring_donations(0.0),
      one_time_donations(0.0) {}

ReportBalanceProperties::ReportBalanceProperties(
    const ReportBalanceProperties& properties) {
  grants = properties.grants;
  ad_earnings = properties.ad_earnings;
  auto_contributions = properties.auto_contributions;
  recurring_donations = properties.recurring_donations;
  one_time_donations = properties.one_time_donations;
}

ReportBalanceProperties::~ReportBalanceProperties() = default;

bool ReportBalanceProperties::operator==(
    const ReportBalanceProperties& rhs) const {
  return grants == rhs.grants && ad_earnings == rhs.ad_earnings &&
         auto_contributions == rhs.auto_contributions &&
         recurring_donations == rhs.recurring_donations &&
         one_time_donations == rhs.one_time_donations;
}

bool ReportBalanceProperties::operator!=(
    const ReportBalanceProperties& rhs) const {
  return !(*this == rhs);
}

base::Value::Dict ReportBalanceProperties::ToValue() const {
  base::Value::Dict root;
  root.Set(kGrantsKey, grants);
  root.Set(kAdEarningsKey, ad_earnings);
  root.Set(kAutoContributionsKey, auto_contributions);
  root.Set(kRecurringDonationsKey, recurring_donations);
  root.Set(kOneTimeDonationsKey, one_time_donations);
  return root;
}

bool ReportBalanceProperties::FromValue(const base::Value::Dict& dict) {
  // Grants
  bool result = GetPropertyFromDict(dict, kGrantsKey, &grants);

  if (!result) {
    NOTREACHED_IN_MIGRATION();
    return false;
  }

  // Earnings From Ads
  result = GetPropertyFromDict(dict, kAdEarningsKey, &ad_earnings);

  if (!result) {
    NOTREACHED_IN_MIGRATION();
    return false;
  }

  // Auto Contribute
  result =
      GetPropertyFromDict(dict, kAutoContributionsKey, &auto_contributions);

  if (!result) {
    NOTREACHED_IN_MIGRATION();
    return false;
  }

  // Recurring Donation
  result =
      GetPropertyFromDict(dict, kRecurringDonationsKey, &recurring_donations);

  if (!result) {
    NOTREACHED_IN_MIGRATION();
    return false;
  }

  // One Time Donation
  result = GetPropertyFromDict(dict, kOneTimeDonationsKey, &one_time_donations);

  if (!result) {
    NOTREACHED_IN_MIGRATION();
    return false;
  }

  return true;
}

std::string ReportBalanceProperties::ToJson() const {
  std::string json;
  CHECK(base::JSONWriter::Write(ToValue(), &json));
  return json;
}

bool ReportBalanceProperties::FromJson(const std::string& json) {
  auto document = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);

  if (!document.has_value()) {
    LOG(ERROR) << "Invalid report balance properties. json=" << json
               << ", error line=" << document.error().line
               << ", error column=" << document.error().column
               << ", error message=" << document.error().message;
    return false;
  }

  const base::Value::Dict* root = document->GetIfDict();
  if (!root) {
    LOG(ERROR) << "Invalid report balance properties. json=" << json;
    return false;
  }

  return FromValue(*root);
}

}  // namespace brave_rewards::internal
