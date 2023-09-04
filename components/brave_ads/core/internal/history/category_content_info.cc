/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/category_content_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(const CategoryContentInfo& lhs,
                const CategoryContentInfo& rhs) {
  const auto tie = [](const CategoryContentInfo& category_content) {
    return std::tie(category_content.category,
                    category_content.user_reaction_type);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const CategoryContentInfo& lhs,
                const CategoryContentInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
