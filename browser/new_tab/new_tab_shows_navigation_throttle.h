/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NEW_TAB_NEW_TAB_SHOWS_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_NEW_TAB_NEW_TAB_SHOWS_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"
#include "url/gurl.h"

class NewTabShowsNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit NewTabShowsNavigationThrottle(
      content::NavigationHandle* navigation_handle);
  ~NewTabShowsNavigationThrottle() override;

  NewTabShowsNavigationThrottle(const NewTabShowsNavigationThrottle&) = delete;
  NewTabShowsNavigationThrottle& operator=(
      const NewTabShowsNavigationThrottle&) = delete;

  static std::unique_ptr<NewTabShowsNavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* navigation_handle);

 private:
  // content::NavigationThrottle overrides:
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

  void LoadNewTabOptionsURL();

  GURL new_tab_options_url_;
  base::WeakPtrFactory<NewTabShowsNavigationThrottle> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_NEW_TAB_NEW_TAB_SHOWS_NAVIGATION_THROTTLE_H_
