/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/renderer_context_menu/brave_render_view_context_menu_mac_cocoa.h"

#include "base/functional/bind.h"
#include "brave/browser/ui/gestures/widget/gesture_context_menu_util.h"
#include "content/public/browser/web_contents.h"

BraveRenderViewContextMenuMacCocoa::BraveRenderViewContextMenuMacCocoa(
    content::RenderFrameHost& render_frame_host,
    const content::ContextMenuParams& params,
    bool is_paste_enabled,
    bool is_paste_and_match_style_enabled,
    NSView* parent_view)
    : RenderViewContextMenuMacCocoa(render_frame_host,
                                    params,
                                    is_paste_enabled,
                                    is_paste_and_match_style_enabled,
                                    parent_view) {}

BraveRenderViewContextMenuMacCocoa::~BraveRenderViewContextMenuMacCocoa() =
    default;

void BraveRenderViewContextMenuMacCocoa::Show() {
  if (auto* web_contents = GetWebContents();
      web_contents &&
      gestures::ConsumeSuppressContextMenuForWebContents(
          web_contents,
          base::BindOnce(&BraveRenderViewContextMenuMacCocoa::ShowNow,
                         weak_factory_.GetWeakPtr()))) {
    return;
  }
  ShowNow();
}

void BraveRenderViewContextMenuMacCocoa::ShowNow() {
  RenderViewContextMenuMacCocoa::Show();
}
