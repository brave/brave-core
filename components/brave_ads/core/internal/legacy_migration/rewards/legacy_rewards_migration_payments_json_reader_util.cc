/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_payments_json_reader_util.h"

#include <string>

#include "base/strings/string_number_conversions.h"

namespace brave_ads::rewards::json::reader {

namespace {

constexpr char kAdsRewardsKey[] = "ads_rewards";
constexpr char kPaymentListKey[] = "payments";
constexpr char kBalanceKey[] = "balance";
constexpr char kMonthKey[] = "month";
constexpr char kTransactionCountKey[] = "transaction_count";

absl::optional<PaymentInfo> ParsePayment(const base::Value::Dict& dict) {
  PaymentInfo payment;

  // Balance
  const absl::optional<double> balance = dict.FindDouble(kBalanceKey);
  if (!balance) {
    return absl::nullopt;
  }
  payment.balance = *balance;

  // Month
  const std::string* const month = dict.FindString(kMonthKey);
  if (!month) {
    return absl::nullopt;
  }
  payment.month = *month;

  // Transaction count
  const std::string* const transaction_count =
      dict.FindString(kTransactionCountKey);
  if (!transaction_count) {
    return absl::nullopt;
  }

  if (!base::StringToInt(*transaction_count, &payment.transaction_count)) {
    return absl::nullopt;
  }

  return payment;
}

absl::optional<PaymentList> GetPaymentsFromList(const base::Value::List& list) {
  PaymentList payments;

  for (const auto& item : list) {
    const auto* item_dict = item.GetIfDict();
    if (!item_dict) {
      return absl::nullopt;
    }

    const absl::optional<PaymentInfo> payment = ParsePayment(*item_dict);
    if (!payment) {
      return absl::nullopt;
    }

    payments.push_back(*payment);
  }

  return payments;
}

}  // namespace

absl::optional<PaymentList> ParsePayments(const base::Value::Dict& dict) {
  const auto* const ads_rewards_dict = dict.FindDict(kAdsRewardsKey);
  if (!ads_rewards_dict) {
    return PaymentList{};
  }

  const auto* const list = ads_rewards_dict->FindList(kPaymentListKey);
  if (!list) {
    return absl::nullopt;
  }

  return GetPaymentsFromList(*list);
}

}  // namespace brave_ads::rewards::json::reader
