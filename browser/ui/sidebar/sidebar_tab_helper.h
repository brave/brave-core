/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_TAB_HELPER_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_TAB_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace sidebar {

// Helper to launch the Leo panel one time.
class SidebarTabHelper : public content::WebContentsUserData<SidebarTabHelper>,
                         public content::WebContentsObserver {
 public:
  static void MaybeCreateForWebContents(content::WebContents* contents);

  ~SidebarTabHelper() override;

 private:
  explicit SidebarTabHelper(content::WebContents* contents);

  // content::WebContentsObserver overrides:
  void PrimaryPageChanged(content::Page& page) override;

  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_TAB_HELPER_H_
