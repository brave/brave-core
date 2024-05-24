/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"

#include "base/feature_list.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_entry.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#endif

BraveLocationBarModelDelegate::BraveLocationBarModelDelegate(Browser* browser)
    : BrowserLocationBarModelDelegate(browser), browser_(browser) {}

BraveLocationBarModelDelegate::~BraveLocationBarModelDelegate() = default;

// static
void BraveLocationBarModelDelegate::FormattedStringFromURL(
    const GURL& url,
    std::u16string* new_formatted_url) {
  if (url.SchemeIs("chrome")) {
    base::ReplaceFirstSubstringAfterOffset(new_formatted_url, 0, u"chrome://",
                                           u"brave://");
  }

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  if (url.SchemeIs(kChromeExtensionScheme) &&
      url.host() == kEthereumRemoteClientExtensionId) {
    base::ReplaceFirstSubstringAfterOffset(
        new_formatted_url, 0, kEthereumRemoteClientBaseUrl, u"brave://wallet");
    base::ReplaceFirstSubstringAfterOffset(new_formatted_url, 0,
                                           kEthereumRemoteClientPhishingUrl,
                                           u"brave://wallet");
    base::ReplaceFirstSubstringAfterOffset(new_formatted_url, 0,
                                           kEthereumRemoteClientEnsRedirectUrl,
                                           u"brave://wallet");
  }
#endif
}

std::u16string
BraveLocationBarModelDelegate::FormattedStringWithEquivalentMeaning(
    const GURL& url,
    const std::u16string& formatted_url) const {
  std::u16string new_formatted_url =
      BrowserLocationBarModelDelegate::FormattedStringWithEquivalentMeaning(
          url, formatted_url);
  BraveLocationBarModelDelegate::FormattedStringFromURL(url,
                                                        &new_formatted_url);
  return new_formatted_url;
}

bool BraveLocationBarModelDelegate::GetURL(GURL* url) const {
#if !BUILDFLAG(IS_ANDROID)
  if (base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs) &&
      browser_->profile()->GetPrefs()->GetBoolean(
          brave_tabs::kSharedPinnedTab)) {
    content::NavigationEntry* entry = GetNavigationEntry();
    if (entry && entry->IsInitialEntry()) {
      auto* active_web_contents = GetActiveWebContents();
      auto* shared_pinned_tab_service =
          SharedPinnedTabServiceFactory::GetForProfile(
              Profile::FromBrowserContext(
                  GetActiveWebContents()->GetBrowserContext()));
      DCHECK(shared_pinned_tab_service);
      if (shared_pinned_tab_service->IsDummyContents(active_web_contents)) {
        // Override visible url for dummy contents so that about:blank is not
        // shown in the location bar.
        // In case of new tab, we don't want it to be shown. But other
        // chrome:// scheme should be visible.
        *url = entry->GetVirtualURL().spec() == "chrome://newtab/"
                   ? GURL()
                   : entry->GetVirtualURL();
        return true;
      }
    }
  }
#endif  // !BUILDFLAG(IS_ANDROID)

  return ChromeLocationBarModelDelegate::GetURL(url);
}
