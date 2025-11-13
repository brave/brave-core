/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "brave/browser/ui/brave_scheme_utils.h"
#include "brave/browser/ui/page_info/features.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "extensions/buildflags/buildflags.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#endif

BraveLocationBarModelDelegate::BraveLocationBarModelDelegate(
    TabStripModel* tab_strip_model)
    : BrowserLocationBarModelDelegate(tab_strip_model),
      tab_strip_model_(tab_strip_model) {}

BraveLocationBarModelDelegate::~BraveLocationBarModelDelegate() = default;

// static
void BraveLocationBarModelDelegate::FormattedStringFromURL(
    const GURL& url,
    std::u16string* new_formatted_url) {
  // Replace chrome:// with brave://
  brave_utils::ReplaceChromeToBraveScheme(*new_formatted_url);
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
  if (base::FeatureList::IsEnabled(tabs::kBraveSharedPinnedTabs) &&
      tab_strip_model_->profile()->GetPrefs()->GetBoolean(
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

const gfx::VectorIcon* BraveLocationBarModelDelegate::GetVectorIconOverride()
    const {
  // Defer to the Chromium implementation first.
  const gfx::VectorIcon* parent_icon =
      BrowserLocationBarModelDelegate::GetVectorIconOverride();
  if (parent_icon) {
    return parent_icon;
  }

  // Only override the icon if we are on a secure page.
  if (GetSecurityLevel() != security_state::SECURE) {
    return nullptr;
  }

  auto* fallback_icon = &kLeoTuneSmallIcon;

  // Return a fallback icon if the Shields/Page Info integration feature is
  // disabled.
  if (!page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    return fallback_icon;
  }

  // Return a fallback icon if we can't determine the shields status.
  content::WebContents* web_contents = GetActiveWebContents();
  if (!web_contents) {
    return fallback_icon;
  }
  auto* shields_helper =
      brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents);
  if (!shields_helper) {
    return fallback_icon;
  }

  // Return the appropriate shields icon based on the Shields status.
  return shields_helper->GetBraveShieldsEnabled()
             ? &kLeoShieldDoneIcon
             : &kLeoShieldDisableFilledIcon;
}
