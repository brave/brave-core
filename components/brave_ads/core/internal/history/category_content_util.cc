/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/category_content_util.h"

#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/public/history/category_content_info.h"

namespace brave_ads {

CategoryContentInfo BuildCategoryContent(const std::string& segment) {
  CategoryContentInfo category_content;

  category_content.user_reaction_type =
      ClientStateManager::GetInstance().GetUserReactionTypeForSegment(segment);
  category_content.category = segment;

  return category_content;
}

}  // namespace brave_ads
