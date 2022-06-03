/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/statement_info.h"

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ads/internal/base/number_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

namespace {

base::Time GetNextPaymentDateFromDictionary(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  const std::string* value = dictionary->FindStringKey("next_payment_date");
  if (!value) {
    return base::Time();
  }

  double value_as_double = 0.0;
  if (!base::StringToDouble(*value, &value_as_double)) {
    return base::Time();
  }

  return base::Time::FromDoubleT(value_as_double);
}

double GetEarningsThisMonthFromDictionary(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  return dictionary->FindDoubleKey("earnings_this_month").value_or(0.0);
}

double GetEarningsLastMonthFromDictionary(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  return dictionary->FindDoubleKey("earnings_last_month").value_or(0.0);
}

int GetAdsReceivedThisMonthFromDictionary(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  return dictionary->FindIntKey("ads_received_this_month").value_or(0);
}

}  // namespace

StatementInfo::StatementInfo() = default;

StatementInfo::StatementInfo(const StatementInfo& info) = default;

StatementInfo& StatementInfo::operator=(const StatementInfo& info) = default;

StatementInfo::~StatementInfo() = default;

bool StatementInfo::operator==(const StatementInfo& rhs) const {
  return DoubleEquals(next_payment_date.ToDoubleT(),
                      rhs.next_payment_date.ToDoubleT()) &&
         DoubleEquals(earnings_this_month, rhs.earnings_this_month) &&
         DoubleEquals(earnings_last_month, rhs.earnings_last_month) &&
         ads_received_this_month == rhs.ads_received_this_month;
}

bool StatementInfo::operator!=(const StatementInfo& rhs) const {
  return !(*this == rhs);
}

std::string StatementInfo::ToJson() const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  // Next payment date
  dictionary.SetStringKey("next_payment_date",
                          base::NumberToString(next_payment_date.ToDoubleT()));

  // Earnings this month
  dictionary.SetDoubleKey("earnings_this_month", earnings_this_month);

  // Earnings last month
  dictionary.SetDoubleKey("earnings_last_month", earnings_last_month);

  // Ads received this month
  dictionary.SetIntKey("ads_received_this_month", ads_received_this_month);

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

bool StatementInfo::FromJson(const std::string& json) {
  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  next_payment_date = GetNextPaymentDateFromDictionary(dictionary);

  earnings_this_month = GetEarningsThisMonthFromDictionary(dictionary);
  earnings_last_month = GetEarningsLastMonthFromDictionary(dictionary);

  ads_received_this_month = GetAdsReceivedThisMonthFromDictionary(dictionary);

  return true;
}

}  // namespace ads
