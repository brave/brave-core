/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/renderer_context_menu/brave_render_view_context_menu_views.h"

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "brave/browser/ui/gestures/widget/gesture_context_menu_util.h"
#include "content/public/browser/web_contents.h"

BraveRenderViewContextMenuViews::BraveRenderViewContextMenuViews(
    content::RenderFrameHost& render_frame_host,
    const content::ContextMenuParams& params,
    bool is_paste_enabled,
    bool is_paste_and_match_style_enabled)
    : RenderViewContextMenuViews(render_frame_host,
                                 params,
                                 is_paste_enabled,
                                 is_paste_and_match_style_enabled) {}

BraveRenderViewContextMenuViews::~BraveRenderViewContextMenuViews() = default;

// static
std::unique_ptr<RenderViewContextMenuViews>
BraveRenderViewContextMenuViews::Create(
    content::RenderFrameHost& render_frame_host,
    const content::ContextMenuParams& params,
    bool is_paste_enabled,
    bool is_paste_and_match_style_enabled) {
  // Protected ctor.
  return base::WrapUnique(new BraveRenderViewContextMenuViews(
      render_frame_host, params, is_paste_enabled,
      is_paste_and_match_style_enabled));
}

void BraveRenderViewContextMenuViews::Show() {
  if (auto* web_contents = GetWebContents();
      web_contents &&
      gestures::ConsumeSuppressContextMenuForWebContents(
          web_contents,
          base::BindOnce(&BraveRenderViewContextMenuViews::ShowNow,
                         weak_factory_.GetWeakPtr()))) {
    return;
  }
  ShowNow();
}

void BraveRenderViewContextMenuViews::ShowNow() {
  // Removes duplicated separator if any. The duplicated separator may appear
  // in |BraveRenderViewContextMenu::InitMenu| after remove the translate menu
  // item.
  RemoveAdjacentSeparators();
  RenderViewContextMenuViews::Show();
}
