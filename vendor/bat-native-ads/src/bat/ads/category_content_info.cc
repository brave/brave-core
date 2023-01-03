/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/category_content_info.h"

namespace ads {

bool operator==(const CategoryContentInfo& lhs,
                const CategoryContentInfo& rhs) {
  return lhs.category == rhs.category &&
         lhs.opt_action_type == rhs.opt_action_type;
}

bool operator!=(const CategoryContentInfo& lhs,
                const CategoryContentInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace ads
