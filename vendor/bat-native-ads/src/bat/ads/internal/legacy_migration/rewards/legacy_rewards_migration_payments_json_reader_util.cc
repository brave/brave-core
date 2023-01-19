/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_payments_json_reader_util.h"

#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"

namespace ads::rewards::json::reader {

namespace {

constexpr char kAdsRewardsKey[] = "ads_rewards";
constexpr char kPaymentListKey[] = "payments";
constexpr char kBalanceKey[] = "balance";
constexpr char kMonthKey[] = "month";
constexpr char kTransactionCountKey[] = "transaction_count";

absl::optional<PaymentInfo> ParsePayment(const base::Value& value) {
  PaymentInfo payment;

  // Balance
  const absl::optional<double> balance = value.FindDoubleKey(kBalanceKey);
  if (!balance) {
    return absl::nullopt;
  }
  payment.balance = *balance;

  // Month
  const std::string* const month = value.FindStringKey(kMonthKey);
  if (!month) {
    return absl::nullopt;
  }
  payment.month = *month;

  // Transaction count
  const std::string* const transaction_count =
      value.FindStringKey(kTransactionCountKey);
  if (!transaction_count) {
    return absl::nullopt;
  }

  if (!base::StringToInt(*transaction_count, &payment.transaction_count)) {
    return absl::nullopt;
  }

  return payment;
}

absl::optional<PaymentList> GetPaymentsFromList(const base::Value& value) {
  if (!value.is_list()) {
    return absl::nullopt;
  }

  PaymentList payments;

  for (const auto& item : value.GetList()) {
    if (!item.is_dict()) {
      return absl::nullopt;
    }

    const absl::optional<PaymentInfo> payment = ParsePayment(item);
    if (!payment) {
      return absl::nullopt;
    }

    payments.push_back(*payment);
  }

  return payments;
}

}  // namespace

absl::optional<PaymentList> ParsePayments(const base::Value& value) {
  const base::Value* const ads_rewards_value =
      value.FindDictKey(kAdsRewardsKey);
  if (!ads_rewards_value) {
    return PaymentList{};
  }

  const base::Value* const payments_value =
      ads_rewards_value->FindListKey(kPaymentListKey);
  if (!payments_value) {
    return absl::nullopt;
  }

  const absl::optional<PaymentList> payments =
      GetPaymentsFromList(*payments_value);
  if (!payments) {
    return absl::nullopt;
  }

  return *payments;
}

}  // namespace ads::rewards::json::reader
