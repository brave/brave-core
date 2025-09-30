/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/side_panel/side_panel_web_ui_view_utils.h"
#include "brave/common/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_id.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/render_frame_host.h"

#include <chrome/browser/ui/views/side_panel/side_panel_web_ui_view.cc>

bool SidePanelWebUIView::HandleContextMenu(
    content::RenderFrameHost& render_frame_host,
    const content::ContextMenuParams& params) {
  // For AI Chat, allow context menus to show (return false)
  // This enables features like spell check, autocorrect, copy/paste, etc.
  GURL url = contents_wrapper_
                 ? contents_wrapper_->web_contents()->GetLastCommittedURL()
                 : GURL();
  if (brave::ShouldEnableContextMenu(url)) {
    return false;  // Allow context menu
  }

  // For all other side panels, suppress context menu
  return true;
}

void SidePanelWebUIView::AddedToWidget() {
  WebView::AddedToWidget();

  DCHECK(contents_wrapper_->web_contents());

  auto* profile = Profile::FromBrowserContext(
      contents_wrapper_->web_contents()->GetBrowserContext());
  if (profile->GetPrefs()->GetBoolean(kWebViewRoundedCorners)) {
    holder()->SetCornerRadii(
        gfx::RoundedCornersF(BraveContentsViewUtil::GetBorderRadius()));
  }
}
