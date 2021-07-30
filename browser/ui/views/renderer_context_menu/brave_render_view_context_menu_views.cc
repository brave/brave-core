/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/renderer_context_menu/brave_render_view_context_menu_views.h"

BraveRenderViewContextMenuViews::BraveRenderViewContextMenuViews(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params)
    : RenderViewContextMenuViews(render_frame_host, params) {}

BraveRenderViewContextMenuViews::~BraveRenderViewContextMenuViews() = default;

// static
RenderViewContextMenuViews* BraveRenderViewContextMenuViews::Create(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params) {
  return new BraveRenderViewContextMenuViews(render_frame_host, params);
}

void BraveRenderViewContextMenuViews::Show() {
  // Removes duplicated separator if any. The duplicated separator may appear
  // in |BraveRenderViewContextMenu::InitMenu| after remove the translate menu
  // item.
  RemoveAdjacentSeparators();
  RenderViewContextMenuViews::Show();
}
