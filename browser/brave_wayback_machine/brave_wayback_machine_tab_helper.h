/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class BraveWaybackMachineTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<BraveWaybackMachineTabHelper> {
 public:
  // Don't attach to tabs in tor profile.
  static void AttachTabHelperIfNeeded(content::WebContents* contents);

  explicit BraveWaybackMachineTabHelper(content::WebContents* contents);
  ~BraveWaybackMachineTabHelper() override;

  BraveWaybackMachineTabHelper(const BraveWaybackMachineTabHelper&) = delete;
  BraveWaybackMachineTabHelper& operator=(
      const BraveWaybackMachineTabHelper&) = delete;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
 private:
  // content::WebContentsObserver overrides:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void CreateInfoBar(int response_code);

  base::WeakPtrFactory<BraveWaybackMachineTabHelper> weak_factory_;
};

#endif  // BRAVE_BROWSER_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_
