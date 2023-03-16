/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_CATEGORY_CONTENT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_CATEGORY_CONTENT_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/category_content_action_types.h"
#include "brave/components/brave_ads/core/export.h"

namespace brave_ads {

struct ADS_EXPORT CategoryContentInfo final {
  std::string category;
  CategoryContentOptActionType opt_action_type =
      CategoryContentOptActionType::kNone;
};

bool operator==(const CategoryContentInfo& lhs, const CategoryContentInfo& rhs);
bool operator!=(const CategoryContentInfo& lhs, const CategoryContentInfo& rhs);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_CATEGORY_CONTENT_INFO_H_
