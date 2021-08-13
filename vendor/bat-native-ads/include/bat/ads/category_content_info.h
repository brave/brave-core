/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_CATEGORY_CONTENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_CATEGORY_CONTENT_INFO_H_

#include <string>

#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT CategoryContentInfo {
  CategoryContentInfo();
  CategoryContentInfo(const CategoryContentInfo& info);
  ~CategoryContentInfo();

  bool operator==(const CategoryContentInfo& rhs) const;
  bool operator!=(const CategoryContentInfo& rhs) const;

  enum class OptAction { kNone = 0, kOptIn, kOptOut };

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  std::string category;
  OptAction opt_action = OptAction::kNone;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_CATEGORY_CONTENT_INFO_H_
