/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/onion_location_navigation_throttle_delegate.h"

#include <utility>

#include "brave/browser/tor/tor_profile_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"

namespace tor {

namespace {

class TorNavigationInitiator
    : public content::WebContentsUserData<TorNavigationInitiator> {
 public:
  TorNavigationInitiator(content::WebContents* tor_web_contents,
                         content::WebContents* initiator_web_contents)
      : content::WebContentsUserData<TorNavigationInitiator>(*tor_web_contents),
        initiator_web_contents_(initiator_web_contents) {
    DCHECK(initiator_web_contents_);
  }

  bool IsInitiatedBy(content::WebContents* initiator_web_contents) const {
    return initiator_web_contents_ == initiator_web_contents;
  }

 private:
  friend class WebContentsUserData;

  // Used only for pointer comparasion.
  const raw_ptr<content::WebContents, DanglingUntriaged>
      initiator_web_contents_ = nullptr;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(TorNavigationInitiator);

void OpenURLInTor(Browser* browser,
                  const GURL& onion_location,
                  content::WebContents* initiator) {
  if (!browser) {
    return;
  }

  TorNavigationInitiator* nav_initiator = nullptr;
  if (initiator) {
    for (int i = 0; i < browser->tab_strip_model()->count(); ++i) {
      auto* ni = TorNavigationInitiator::FromWebContents(
          browser->tab_strip_model()->GetWebContentsAt(i));
      if (ni && ni->IsInitiatedBy(initiator)) {
        nav_initiator = ni;
        break;
      }
    }
  }

  if (!nav_initiator) {
    // New tab.
    content::OpenURLParams open_tor(onion_location, content::Referrer(),
                                    WindowOpenDisposition::SWITCH_TO_TAB,
                                    ui::PAGE_TRANSITION_TYPED, false);
    auto* tor_web_contents = browser->OpenURL(open_tor);
    if (initiator) {
      TorNavigationInitiator::CreateForWebContents(tor_web_contents, initiator);
    } else {
      tor_web_contents->RemoveUserData(TorNavigationInitiator::UserDataKey());
    }
  } else {
    // Redirect navigation to an existing tab.
    nav_initiator->GetWebContents().GetController().LoadURL(
        onion_location, content::Referrer(), ui::PAGE_TRANSITION_TYPED, {});
    nav_initiator->GetWebContents().NotifyNavigationStateChanged(
        content::INVALIDATE_TYPE_URL);
  }
}

}  // namespace

OnionLocationNavigationThrottleDelegate::
    OnionLocationNavigationThrottleDelegate() = default;

OnionLocationNavigationThrottleDelegate::
    ~OnionLocationNavigationThrottleDelegate() = default;

void OnionLocationNavigationThrottleDelegate::OpenInTorWindow(
    content::WebContents* context,
    const GURL& onion_location,
    bool renderer_initiated) {
  Profile* profile = Profile::FromBrowserContext(context->GetBrowserContext());
  Browser* tor_browser = TorProfileManager::SwitchToTorProfile(profile);
  if (renderer_initiated) {
    OpenURLInTor(tor_browser, onion_location, context);
  } else {
    OpenURLInTor(tor_browser, onion_location, nullptr);
  }
}

}  // namespace tor
