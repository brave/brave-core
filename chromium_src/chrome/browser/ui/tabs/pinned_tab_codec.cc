/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/pinned_tab_codec.h"

#include <optional>
#include <string>

#include "base/feature_list.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/ui/tabs/features.h"
#include "url/gurl.h"
#include "url/url_constants.h"

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

namespace pinned_tab_reset {

// The URL the tab was pinned with, if the pinned tab reset feature is on.
std::optional<GURL> GetPinnedUrl(const content::WebContents* contents);

}  // namespace pinned_tab_reset

namespace {

// The URL the tab was pinned with. Kept apart from upstream's "url", the last
// visited page, so turning the pinned tab reset feature off restores it.
constexpr char kBravePinnedUrl[] = "brave_pinned_url";

bool BraveShouldSkipPinnedTab(BrowserWindowInterface* browser,
                              content::WebContents* web_contents) {
  return sidebar::IsWebPanelTab(browser, web_contents);
}

void BraveModifyEncodedPinnedTab(content::WebContents* web_contents,
                                 base::Value& value) {
  if (const auto pinned_url = pinned_tab_reset::GetPinnedUrl(web_contents)) {
    value.GetDict().Set(kBravePinnedUrl, pinned_url->spec());
  }
#if BUILDFLAG(ENABLE_CONTAINERS)
  containers::SerializeContainerSpecifier(web_contents, value);
#endif
}

void BraveModifyDecodedPinnedTab(const base::Value& value, StartupTab& tab) {
  // With the feature on, the tab opens at the URL it was pinned with, rather
  // than wherever it navigated to.
  if (base::FeatureList::IsEnabled(tabs::kBravePinnedTabReset)) {
    if (const std::string* pinned_url_spec =
            value.GetDict().FindString(kBravePinnedUrl)) {
      const GURL pinned_url(*pinned_url_spec);
      if (pinned_url.is_valid() &&
          !pinned_url.SchemeIs(url::kJavaScriptScheme)) {
        tab.url = pinned_url;
      }
    }
  }
#if BUILDFLAG(ENABLE_CONTAINERS)
  containers::DeserializeContainerSpecifier(value, tab.container);
#endif
}

}  // namespace

#include <chrome/browser/ui/tabs/pinned_tab_codec.cc>
