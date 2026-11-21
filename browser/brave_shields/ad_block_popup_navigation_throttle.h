// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_POPUP_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_POPUP_NAVIGATION_THROTTLE_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_shields/ad_block_popup_util.h"
#include "content/public/browser/navigation_throttle.h"

class HostContentSettingsMap;

namespace brave_shields {

class AdBlockService;

// Closes a window opened by a page if its first navigation matches a `$popup`
// rule, using the opener as the source.
class AdBlockPopupNavigationThrottle : public content::NavigationThrottle {
 public:
  AdBlockPopupNavigationThrottle(content::NavigationThrottleRegistry& registry,
                                 AdBlockService* ad_block_service,
                                 HostContentSettingsMap* content_settings);
  ~AdBlockPopupNavigationThrottle() override;

  AdBlockPopupNavigationThrottle(const AdBlockPopupNavigationThrottle&) =
      delete;
  AdBlockPopupNavigationThrottle& operator=(
      const AdBlockPopupNavigationThrottle&) = delete;

  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry,
                                AdBlockService* ad_block_service,
                                HostContentSettingsMap* content_settings);

  // content::NavigationThrottle implementation:
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;
  const char* GetNameForLogging() override;

 private:
  void ReadOpener();
  void OnShouldBlockPopup(bool should_block);

  const raw_ptr<AdBlockService> ad_block_service_ = nullptr;
  const raw_ptr<HostContentSettingsMap> content_settings_ = nullptr;

  // Looked up on the first request and reused for redirects, since by then the
  // opener may have navigated away or closed.
  bool opener_read_ = false;
  std::optional<PopupOpener> opener_;

  base::WeakPtrFactory<AdBlockPopupNavigationThrottle> weak_ptr_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_POPUP_NAVIGATION_THROTTLE_H_
