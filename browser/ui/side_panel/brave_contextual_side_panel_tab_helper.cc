/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/side_panel/brave_contextual_side_panel_tab_helper.h"

#include "brave/browser/ui/side_panel/brave_side_panel_utils.h"

BraveContextualSidePanelTabHelper::BraveContextualSidePanelTabHelper(
    content::WebContents* contents)
    : WebContentsUserData(*contents) {
  brave::RegisterContextualSidePanel(contents);
}

BraveContextualSidePanelTabHelper::~BraveContextualSidePanelTabHelper() =
    default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveContextualSidePanelTabHelper);
