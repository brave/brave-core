/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/analytics/p2a/p2a_value_util.h"

namespace brave_ads::p2a {

base::Value::List EventsToValue(const std::vector<std::string>& events) {
  base::Value::List list;

  for (const auto& event : events) {
    if (!event.empty()) {
      list.Append(event);
    }
  }

  return list;
}

}  // namespace brave_ads::p2a
