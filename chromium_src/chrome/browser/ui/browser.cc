/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/brave_browser_actions.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/browser/ui/brave_browser_content_setting_bubble_model_delegate.h"
#include "brave/browser/ui/brave_tab_strip_model_delegate.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_content_setting_bubble_model_delegate.h"

#define BRAVE_BROWSER_CREATE return new BraveBrowser(params);

// We have one more option for bookmarks bar visibility.
// It's "only show bookmarks bar on NTP". And it's patched to prevent
// checking split view. Chromium shows bookmarks bar if active tab is
// in split tabs and split tabs includes NTP. But, we don't want to show
// bookmarks bar if that active tab is not NTP.
// It's inevitable to copy some upstream code when overriding this method.
// Also added this before checking bookmarks/tab groups existence.
// We show bookmarks bar on NTP if there is no bookmarks/tab groups.
#define BRAVE_BROWSER_SHOULD_SHOW_BOOKMARK_BAR      \
  return IsShowingNTP(active_tab->GetContents()) && \
         prefs->GetBoolean(kAlwaysShowBookmarkBarOnNTP);

#define BrowserContentSettingBubbleModelDelegate \
  BraveBrowserContentSettingBubbleModelDelegate
#define BrowserCommandController BraveBrowserCommandController
#define BrowserTabStripModelDelegate BraveTabStripModelDelegate
#define BrowserActions(...) BraveBrowserActions(__VA_ARGS__)
#define DeprecatedCreateOwnedForTesting DeprecatedCreateOwnedForTesting_Unused

#include "src/chrome/browser/ui/browser.cc"

#undef DeprecatedCreateOwnedForTesting
#undef BrowserActions
#undef BrowserTabStripModelDelegate
#undef BrowserContentSettingBubbleModelDelegate
#undef BrowserCommandController
#undef BRAVE_BROWSER_SHOULD_SHOW_BOOKMARK_BAR
#undef BRAVE_BROWSER_DEPRECATED_CREATE_OWNED_FOR_TESTING
#undef BRAVE_BROWSER_CREATE

bool IsShowingNTP_ChromiumImpl(content::WebContents* web_contents) {
  return IsShowingNTP(web_contents);
}

// static
std::unique_ptr<Browser> Browser::DeprecatedCreateOwnedForTesting(
    const CreateParams& params) {
  CHECK_IS_TEST();
  // If this is failing, a caller is trying to create a browser when creation is
  // not possible, e.g. using the wrong profile or during shutdown. The caller
  // should handle this; see e.g. crbug.com/1141608 and crbug.com/1261628.
  CHECK_EQ(CreationStatus::kOk, GetCreationStatusForProfile(params.profile));
  return base::WrapUnique(new BraveBrowser(params));
}
