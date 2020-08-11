/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/string_split.h"
#include "bat/ledger/internal/legacy/bat_util.h"

namespace braveledger_bat_util {

std::string ConvertToProbi(const std::string& amount) {
  if (amount.empty()) {
    return "0";
  }

  auto vec = base::SplitString(
      amount, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

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

  return std::stod(amount);
}

}  // namespace braveledger_bat_util
