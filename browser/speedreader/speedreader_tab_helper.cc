/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_tab_helper.h"

#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/speedreader_button.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"

namespace speedreader {

SpeedreaderTabHelper::~SpeedreaderTabHelper() = default;

SpeedreaderTabHelper::SpeedreaderTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

void SpeedreaderTabHelper::SetActive() {
  active_ = true;
  UpdateButton();
}

void SpeedreaderTabHelper::OnVisibilityChanged(content::Visibility visibility) {
  if (visibility != content::Visibility::HIDDEN) {
    UpdateButton();
  }
}

void SpeedreaderTabHelper::UpdateButton() {
  // TODO(iefremov): We probably should do it with lesser amount of hacks.
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  BrowserView* view = static_cast<BrowserView*>(browser->window());
  SpeedreaderButton* button =
      static_cast<BraveToolbarView*>(view->toolbar())->speedreader_button();
  if (button) {
    button->SetActive(active_);
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SpeedreaderTabHelper)

}  // namespace speedreader
