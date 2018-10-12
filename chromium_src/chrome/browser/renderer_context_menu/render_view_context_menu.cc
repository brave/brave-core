// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"

// Our .h file creates a masquerade for RenderViewContextMenu.  Switch
// back to the Chromium one for the Chromium implementation.
#undef RenderViewContextMenu
#define RenderViewContextMenu RenderViewContextMenu_Chromium

#include "../../../../chrome/browser/renderer_context_menu/render_view_context_menu.cc"

BraveRenderViewContextMenu::BraveRenderViewContextMenu(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params)
  : RenderViewContextMenu_Chromium(render_frame_host, params) {
}

bool BraveRenderViewContextMenu::IsCommandIdEnabled(int id) const {
  switch (id) {
    case IDC_CONTENT_CONTEXT_OPENLINKTOR:
      return params_.link_url.is_valid() && !browser_context_->IsTorProfile();
    default:
      return RenderViewContextMenu_Chromium::IsCommandIdEnabled(id);
  }
}

