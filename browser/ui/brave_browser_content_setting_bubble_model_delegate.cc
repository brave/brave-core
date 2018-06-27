/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_content_setting_bubble_model_delegate.h"

#include "brave/common/url_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"

BraveBrowserContentSettingBubbleModelDelegate::
BraveBrowserContentSettingBubbleModelDelegate(Browser* browser) :
    BrowserContentSettingBubbleModelDelegate(browser),
    browser_(browser) {
}

BraveBrowserContentSettingBubbleModelDelegate::
~BraveBrowserContentSettingBubbleModelDelegate() {
}

void
BraveBrowserContentSettingBubbleModelDelegate::ShowWidevineLearnMorePage() {
  GURL learn_more_url = GURL(kWidevineTOS);
  chrome::AddSelectedTabWithURL(browser_, learn_more_url,
                                ui::PAGE_TRANSITION_LINK);
}
