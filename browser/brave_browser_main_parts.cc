/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_parts.h"
#include "brave/browser/browsing_data/brave_clear_browsing_data.h"
#include "brave/browser/infobars/brave_confirm_p3a_infobar_delegate.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/browser_finder.h"

void BraveBrowserMainParts::PostBrowserStart() {
  Browser* browser = chrome::FindLastActive();
  content::WebContents* active_web_contents = nullptr;

  if (browser) {
    active_web_contents = browser->tab_strip_model()->GetActiveWebContents();

    if (active_web_contents) {
      InfoBarService* infobar_service =
          InfoBarService::FromWebContents(active_web_contents);

      if (infobar_service) {
        BraveConfirmP3AInfoBarDelegate::Create(infobar_service,
            g_browser_process->local_state());
      }
    }
  }
}

void BraveBrowserMainParts::PreShutdown() {
  content::BraveClearBrowsingData::ClearOnExit();
}
