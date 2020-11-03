/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CATEGORY_CONTENT_H_
#define BAT_ADS_CATEGORY_CONTENT_H_

#include <string>

#include "bat/ads/export.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT CategoryContentInfo {
  CategoryContentInfo();
  CategoryContentInfo(
      const CategoryContentInfo& info);
  ~CategoryContentInfo();

  bool operator==(
      const CategoryContentInfo& rhs) const;
  bool operator!=(
      const CategoryContentInfo& rhs) const;

  enum class OptAction {
    kNone = 0,
    kOptIn,
    kOptOut
  };

  std::string ToJson() const;
  Result FromJson(
      const std::string& json);

  std::string category;
  OptAction opt_action = OptAction::kNone;
};

}  // namespace ads

#endif  // BAT_ADS_CATEGORY_CONTENT_H_
