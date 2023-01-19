/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/category_content_util.h"

#include "bat/ads/category_content_info.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"

namespace ads {

CategoryContentInfo BuildCategoryContent(const std::string& segment) {
  CategoryContentInfo category_content;

  category_content.opt_action_type =
      ClientStateManager::GetInstance()
          ->GetCategoryContentOptActionTypeForSegment(segment);
  category_content.category = segment;

  return category_content;
}

}  // namespace ads
