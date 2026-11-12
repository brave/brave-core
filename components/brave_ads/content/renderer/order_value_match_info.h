/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_VALUE_MATCH_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_VALUE_MATCH_INFO_H_

#include <optional>
#include <string>

namespace brave_ads {

// The generic heuristic's matched value text, and the currency code (e.g.
// "EUR") whose regex matched it, if any; see `MatchCurrency`.
struct OrderValueMatchInfo final {
  std::string value;
  std::optional<std::string> currency;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_VALUE_MATCH_INFO_H_
