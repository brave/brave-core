/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/pinned_tab_codec.h"

#include "brave/components/containers/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/containers/container_specifier_utils.h"
#endif

class BrowserWindowInterface;

namespace content {
class WebContents;
}  // namespace content

namespace sidebar {

// A sidebar web panel is backed by a pinned tab, but it isn't one of the user's
// pinned tabs, so it must not be written to prefs::kPinnedTabs - otherwise the
// next launch reopens it as an ordinary pinned tab. Filtered here so every
// PinnedTabCodec::WritePinnedTabs() caller is covered, including the OS session
// end path, which never closes tabs.
bool IsWebPanelTab(BrowserWindowInterface* browser,
                   const content::WebContents* contents);

}  // namespace sidebar

namespace {

bool BraveShouldSkipPinnedTab(BrowserWindowInterface* browser,
                              content::WebContents* web_contents) {
  return sidebar::IsWebPanelTab(browser, web_contents);
}

void BraveModifyEncodedPinnedTab(content::WebContents* web_contents,
                                 base::Value& value) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  containers::SerializeContainerSpecifier(web_contents, value);
#endif
}

void BraveModifyDecodedPinnedTab(const base::Value& value, StartupTab& tab) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  containers::DeserializeContainerSpecifier(value, tab.container);
#endif
}

}  // namespace

#include <chrome/browser/ui/tabs/pinned_tab_codec.cc>
