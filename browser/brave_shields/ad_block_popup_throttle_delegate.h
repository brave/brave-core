// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_POPUP_THROTTLE_DELEGATE_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_POPUP_THROTTLE_DELEGATE_H_

#include "brave/components/brave_shields/content/browser/ad_block_popup_navigation_throttle.h"

namespace brave_shields {

// Counts a popup blocked by a `$popup` rule as a blocked ad, and lists it in
// the opener tab's blocked-popup UI so the user can still open it.
class AdBlockPopupThrottleDelegate
    : public AdBlockPopupNavigationThrottle::Delegate {
 public:
  AdBlockPopupThrottleDelegate();
  AdBlockPopupThrottleDelegate(const AdBlockPopupThrottleDelegate&) = delete;
  AdBlockPopupThrottleDelegate& operator=(const AdBlockPopupThrottleDelegate&) =
      delete;
  ~AdBlockPopupThrottleDelegate() override;

  // AdBlockPopupNavigationThrottle::Delegate:
  void OnPopupBlocked(const AdBlockPopupOpener& opener,
                      content::NavigationHandle& handle) override;
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_POPUP_THROTTLE_DELEGATE_H_
