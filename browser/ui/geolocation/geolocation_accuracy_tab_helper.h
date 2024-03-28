/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_ACCURACY_TAB_HELPER_H_
#define BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_ACCURACY_TAB_HELPER_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class GeolocationAccuracyTabHelper
    : public content::WebContentsUserData<GeolocationAccuracyTabHelper>,
      public content::WebContentsObserver {
 public:
  static void MaybeCreateForWebContents(content::WebContents* contents);

  ~GeolocationAccuracyTabHelper() override;

  // Launches per-tab dialog.
  void LaunchAccuracyHelperDialogIfNeeded();

  // content::WebContentsObserver overrides:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  friend WebContentsUserData;
  FRIEND_TEST_ALL_PREFIXES(GeolocationAccuracyBrowserTest, DialogLaunchTest);
  FRIEND_TEST_ALL_PREFIXES(GeolocationAccuracyBrowserTest,
                           DialogLaunchDisabledTest);

  explicit GeolocationAccuracyTabHelper(content::WebContents* contents);

  void OnDialogClosed();

  bool dialog_asked_in_current_navigation_ = false;
  bool is_dialog_running_ = false;

  base::WeakPtrFactory<GeolocationAccuracyTabHelper> weak_ptr_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_ACCURACY_TAB_HELPER_H_
