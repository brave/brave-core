/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_EXTRACTED_ORDER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_EXTRACTED_ORDER_INFO_H_

#include <optional>
#include <string>

namespace brave_ads {

struct ExtractedOrderInfo final {
  bool operator==(const ExtractedOrderInfo&) const = default;

  std::optional<std::string> id;
  std::optional<std::string> value;
  std::optional<std::string> currency;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_EXTRACTED_ORDER_INFO_H_
