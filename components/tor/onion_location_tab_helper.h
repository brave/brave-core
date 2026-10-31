/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_ONION_LOCATION_TAB_HELPER_H_
#define BRAVE_COMPONENTS_TOR_ONION_LOCATION_TAB_HELPER_H_

#include <optional>

#include "base/callback_list.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace tor {

// Holds the Onion-Location of the page shown in a tab. Owned by the tab's
// features, and follows the tab's contents across discards.
class OnionLocationTabHelper : public content::WebContentsObserver {
 public:
  DECLARE_USER_DATA(OnionLocationTabHelper);

  static OnionLocationTabHelper* From(tabs::TabInterface* tab);

  explicit OnionLocationTabHelper(tabs::TabInterface& tab);
  OnionLocationTabHelper(const OnionLocationTabHelper&) = delete;
  OnionLocationTabHelper& operator=(const OnionLocationTabHelper&) = delete;
  ~OnionLocationTabHelper() override;

  bool should_show_icon() const { return !onion_location_.is_empty(); }

  const GURL& onion_location() const { return onion_location_; }

  const std::optional<url::Origin>& initiator_origin() const {
    return initiator_origin_;
  }

  void SetOnionLocationForTesting(
      const GURL& onion_location,
      std::optional<url::Origin> initiator_origin = std::nullopt);

 private:
  friend class OnionLocationNavigationThrottle;

  static void SetOnionLocationByThrottle(
      content::WebContents* web_contents,
      const GURL& onion_location,
      const std::optional<url::Origin>& initiator_origin = std::nullopt);

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents);

  GURL throttle_reported_onion_location_;
  std::optional<url::Origin> throttle_reported_initiator_origin_;
  GURL onion_location_;
  std::optional<url::Origin> initiator_origin_;

  base::CallbackListSubscription will_discard_contents_subscription_;
  ui::ScopedUnownedUserData<OnionLocationTabHelper> scoped_unowned_user_data_;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_ONION_LOCATION_TAB_HELPER_H_
