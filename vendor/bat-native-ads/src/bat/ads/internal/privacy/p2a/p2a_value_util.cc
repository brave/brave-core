/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/p2a/p2a_value_util.h"

namespace ads::privacy::p2a {

base::Value::List QuestionsToValue(const std::vector<std::string>& questions) {
  base::Value::List list;

  for (const auto& question : questions) {
    if (!question.empty()) {
      list.Append(question);
    }
  }

  return list;
}

}  // namespace ads::privacy::p2a
