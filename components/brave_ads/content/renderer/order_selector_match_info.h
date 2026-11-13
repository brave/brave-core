/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_SELECTOR_MATCH_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_SELECTOR_MATCH_INFO_H_

#include <optional>
#include <string>

namespace brave_ads {

struct OrderSelectorMatchInfo {
  // The value extracted by the selector.
  std::string value;

  // The optional validation pattern associated with the selector.
  std::optional<std::string> validation_pattern;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_ORDER_SELECTOR_MATCH_INFO_H_
