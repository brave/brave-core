/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_CATEGORY_CONTENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_CATEGORY_CONTENT_INFO_H_

#include <string>

#include "base/values.h"
#include "bat/ads/category_content_action_types.h"
#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT CategoryContentInfo final {
  CategoryContentInfo();
  CategoryContentInfo(const CategoryContentInfo& info);
  CategoryContentInfo& operator=(const CategoryContentInfo& info);
  ~CategoryContentInfo();

  bool operator==(const CategoryContentInfo& rhs) const;
  bool operator!=(const CategoryContentInfo& rhs) const;

  base::Value::Dict ToValue() const;
  void FromValue(const base::Value::Dict& value);

  std::string category;
  CategoryContentOptActionType opt_action_type =
      CategoryContentOptActionType::kNone;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_CATEGORY_CONTENT_INFO_H_
