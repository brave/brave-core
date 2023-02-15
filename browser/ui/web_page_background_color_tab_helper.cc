/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/web_page_background_color_tab_helper.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "content/public/browser/navigation_entry.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider.h"
#include "url/gurl.h"

namespace {

bool IsNTP(content::WebContents* web_contents) {
  content::NavigationEntry* entry =
      web_contents->GetController().GetLastCommittedEntry();
  if (!entry) {
    entry = web_contents->GetController().GetVisibleEntry();
    return false;
  }

  const GURL url = entry->GetURL();
  return NewTabUI::IsNewTab(url) || NewTabPageUI::IsNewTabPageOrigin(url) ||
         search::NavEntryIsInstantNTP(web_contents, entry);
}

absl::optional<SkColor> GetNTPBackgroundColor(content::WebContents* contents) {
  // Get the specific background color for the type of browser window
  // that the contents is in.
  // TODO(petemill): we do not use web_contents->GetColorProvider()
  // here because it does not include BravePrivateWindowThemeSupplier. This
  // should get fixed, potentially via `WebContents::SetColorProviderSource`.
  auto* browser_window =
      BrowserWindow::FindBrowserWindowWithWebContents(contents);
  if (!browser_window) {
    // Some newly created WebContents aren't yet attached
    // to a browser window, so get any that match the current profile,
    // which is fine for color provider.
    Profile* profile =
        Profile::FromBrowserContext(contents->GetBrowserContext());
    const Browser* browser = chrome::FindBrowserWithProfile(profile);
    if (browser) {
      browser_window = browser->window();
    }
  }
  if (!browser_window) {
    DLOG(ERROR) << "No BrowserWindow could be found for WebContents";
    return absl::nullopt;
  }

  const ui::ColorProvider* color_provider = browser_window->GetColorProvider();
  return color_provider->GetColor(kColorNewTabPageBackground);
}

}  // namespace

WebPageBackgroundColorTabHelper::WebPageBackgroundColorTabHelper(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<WebPageBackgroundColorTabHelper>(
          *web_contents) {}

WebPageBackgroundColorTabHelper::~WebPageBackgroundColorTabHelper() = default;

void WebPageBackgroundColorTabHelper::PrimaryPageChanged(content::Page& page) {
  web_contents()->SetPageBaseBackgroundColor(absl::nullopt);

  if (IsNTP(web_contents())) {
    web_contents()->SetPageBaseBackgroundColor(
        GetNTPBackgroundColor(web_contents()));
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebPageBackgroundColorTabHelper);
