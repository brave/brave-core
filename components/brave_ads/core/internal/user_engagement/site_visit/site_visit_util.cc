/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_util.h"

#include "brave/components/brave_ads/core/internal/common/url/url_util.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "url/gurl.h"

namespace brave_ads {

bool DidLandOnPage(const int32_t tab_id, const GURL& url) {
  const std::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetForId(tab_id);
  if (!tab) {
    // The tab has been closed.
    return false;
  }

  return DomainOrHostExists(tab->redirect_chain, url);
}

}  // namespace brave_ads
