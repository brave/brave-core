/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/legacy/bat_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"

namespace brave_rewards::internal {

std::string ConvertToProbi(const std::string& amount) {
  if (amount.empty()) {
    return "0";
  }

  auto vec = base::SplitString(amount, ".", base::TRIM_WHITESPACE,
                               base::SPLIT_WANT_NONEMPTY);

  const std::string probi = "000000000000000000";

  if (vec.size() == 1) {
    return vec.at(0) + probi;
  }

  const auto before_dot = vec.at(0);
  const auto after_dot = vec.at(1);
  const auto rest_probi = probi.substr(after_dot.size());

  return before_dot + after_dot + rest_probi;
}

double ProbiToDouble(const std::string& probi) {
  const size_t size = probi.size();
  std::string amount = "0";
  if (size > 18) {
    amount = probi;
    amount.insert(size - 18, ".");
  }

  double value = 0;
  base::StringToDouble(amount, &value);
  return value;
}

}  // namespace brave_rewards::internal
