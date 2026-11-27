// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_POPUP_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_POPUP_NAVIGATION_THROTTLE_H_

#include <memory>
#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/navigation_throttle.h"
#include "url/origin.h"

namespace base {
class Clock;
}  // namespace base

namespace content {
class NavigationHandle;
}  // namespace content

namespace brave_shields {

class AdBlockService;
class BraveShieldsSettingsService;

// The frame a popup's navigation is checked for.
struct AdBlockPopupOpener {
  // Not set if the frame isn't known.
  std::optional<content::GlobalRenderFrameHostToken> frame_token;
  // The source of the popup's requests.
  url::Origin origin;
  bool aggressive = false;
};

// Holds each request of a popup until the ad block engine has checked it
// against `$popup` rules. On a match, cancels it and closes the popup.
class AdBlockPopupNavigationThrottle : public content::NavigationThrottle {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when a popup is blocked, with its navigation at the URL that
    // matched.
    virtual void OnPopupBlocked(const AdBlockPopupOpener& opener,
                                content::NavigationHandle& handle) = 0;
  };

  AdBlockPopupNavigationThrottle(content::NavigationThrottleRegistry& registry,
                                 AdBlockService* ad_block_service,
                                 AdBlockPopupOpener opener,
                                 std::unique_ptr<Delegate> delegate);
  ~AdBlockPopupNavigationThrottle() override;

  AdBlockPopupNavigationThrottle(const AdBlockPopupNavigationThrottle&) =
      delete;
  AdBlockPopupNavigationThrottle& operator=(
      const AdBlockPopupNavigationThrottle&) = delete;

  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry,
                                AdBlockService* ad_block_service,
                                BraveShieldsSettingsService* settings_service,
                                std::unique_ptr<Delegate> delegate);

  static void SetClockForTesting(const base::Clock* clock);

  // content::NavigationThrottle:
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  void OnShouldBlock(bool should_block);

  const AdBlockPopupOpener opener_;
  const std::unique_ptr<Delegate> delegate_;
  const raw_ptr<AdBlockService> ad_block_service_;

  base::WeakPtrFactory<AdBlockPopupNavigationThrottle> weak_ptr_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_POPUP_NAVIGATION_THROTTLE_H_
