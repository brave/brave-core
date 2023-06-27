// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/sidebar/sidebar_tab_helper.h"

#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

SidebarTabHelper::SidebarTabHelper(content::WebContents* contents)
    : content::WebContentsUserData<SidebarTabHelper>(*contents) {}

SidebarTabHelper::~SidebarTabHelper() = default;

void SidebarTabHelper::RegisterPanelActive(
    sidebar::SidebarItem::BuiltInItemType panel_id) {
  active_panel_ = panel_id;
}

void SidebarTabHelper::RegisterPanelInactive() {
  active_panel_ = absl::nullopt;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SidebarTabHelper);
