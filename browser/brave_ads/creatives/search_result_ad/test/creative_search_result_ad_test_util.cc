/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/creatives/search_result_ad/test/creative_search_result_ad_test_util.h"

#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_delegate.h"
#include "content/public/browser/web_contents.h"

namespace brave_ads::test {

bool HasSearchResultAdClickedInfoBar(content::WebContents* web_contents) {
  infobars::ContentInfoBarManager* info_bar_manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents);
  if (!info_bar_manager) {
    return false;
  }

  for (infobars::InfoBar* info_bar : info_bar_manager->infobars()) {
    if (info_bar->delegate()->GetIdentifier() ==
        infobars::InfoBarDelegate::SEARCH_RESULT_AD_CLICKED_INFOBAR_DELEGATE) {
      return true;
    }
  }
  return false;
}

}  // namespace brave_ads::test
