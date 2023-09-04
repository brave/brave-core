// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_TAB_HELPER_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_TAB_HELPER_H_

#include "brave/components/sidebar/sidebar_item.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class SidebarTabHelper : public content::WebContentsUserData<SidebarTabHelper> {
 public:
  SidebarTabHelper(const SidebarTabHelper&) = delete;
  SidebarTabHelper& operator=(const SidebarTabHelper&) = delete;

  ~SidebarTabHelper() override;

  void RegisterPanelActive(sidebar::SidebarItem::BuiltInItemType panel_id);
  void RegisterPanelInactive();
  absl::optional<sidebar::SidebarItem::BuiltInItemType> active_panel() const {
    return active_panel_;
  }

 private:
  explicit SidebarTabHelper(content::WebContents* contents);

  // Whether the sidebar panel was explicitly opened for this tab. Some panel
  // types want to only be opened for Tab which they were specifically
  // opened on by the user.
  absl::optional<sidebar::SidebarItem::BuiltInItemType> active_panel_;

  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_TAB_HELPER_H_
