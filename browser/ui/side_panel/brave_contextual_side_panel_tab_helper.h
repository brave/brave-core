/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDE_PANEL_BRAVE_CONTEXTUAL_SIDE_PANEL_TAB_HELPER_H_
#define BRAVE_BROWSER_UI_SIDE_PANEL_BRAVE_CONTEXTUAL_SIDE_PANEL_TAB_HELPER_H_

#include "content/public/browser/web_contents_user_data.h"

// Helper to register contextual side panel entry.
class BraveContextualSidePanelTabHelper
    : public content::WebContentsUserData<BraveContextualSidePanelTabHelper> {
 public:
  explicit BraveContextualSidePanelTabHelper(content::WebContents* contents);
  ~BraveContextualSidePanelTabHelper() override;

 private:
  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_SIDE_PANEL_BRAVE_CONTEXTUAL_SIDE_PANEL_TAB_HELPER_H_
