/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/side_panel_web_ui_view_utils.h"

#include <chrome/browser/ui/views/side_panel/side_panel_web_ui_view.cc>

bool SidePanelWebUIView::HandleContextMenu(
    content::RenderFrameHost& render_frame_host,
    const content::ContextMenuParams& params) {
  // For AI Chat, allow context menus to show (return false)
  // This enables features like spell check, autocorrect, copy/paste, etc.
  GURL url = web_contents()->GetLastCommittedURL();
  if (brave::ShouldEnableContextMenu(url)) {
    return false;  // Allow context menu
  }

  // For all other side panels, suppress context menu
  return true;
}
