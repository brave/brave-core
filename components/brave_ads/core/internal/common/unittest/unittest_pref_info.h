/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_INFO_H_

#include <optional>

#include "base/values.h"

namespace brave_ads {

struct PrefInfo final {
  PrefInfo();

  PrefInfo(const PrefInfo&);
  PrefInfo& operator=(const PrefInfo&);

  PrefInfo(PrefInfo&&) noexcept;
  PrefInfo& operator=(PrefInfo&&) noexcept;

  ~PrefInfo();

  bool operator==(const PrefInfo&) const = default;

  std::optional<base::Value> value;
  base::Value default_value;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_INFO_H_
