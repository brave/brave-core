/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_RENDERER_CONTEXT_MENU_BRAVE_RENDER_VIEW_CONTEXT_MENU_MAC_REMOTE_COCOA_H_
#define BRAVE_BROWSER_UI_VIEWS_RENDERER_CONTEXT_MENU_BRAVE_RENDER_VIEW_CONTEXT_MENU_MAC_REMOTE_COCOA_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/cocoa/renderer_context_menu/render_view_context_menu_mac_remote_cocoa.h"

class BraveRenderViewContextMenuMacRemoteCocoa
    : public RenderViewContextMenuMacRemoteCocoa {
 public:
  BraveRenderViewContextMenuMacRemoteCocoa(
      content::RenderFrameHost& render_frame_host,
      const content::ContextMenuParams& params,
      bool is_paste_enabled,
      bool is_paste_and_match_style_enabled,
      content::RenderWidgetHostView* parent_view);

  BraveRenderViewContextMenuMacRemoteCocoa(
      const BraveRenderViewContextMenuMacRemoteCocoa&) = delete;
  BraveRenderViewContextMenuMacRemoteCocoa& operator=(
      const BraveRenderViewContextMenuMacRemoteCocoa&) = delete;

  ~BraveRenderViewContextMenuMacRemoteCocoa() override;

  void Show() override;

 private:
  // Actually shows the menu, run immediately or deferred; see
  // `gestures::ConsumeSuppressContextMenuForWebContents`.
  void ShowNow();

  base::WeakPtrFactory<BraveRenderViewContextMenuMacRemoteCocoa> weak_factory_{
      this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_RENDERER_CONTEXT_MENU_BRAVE_RENDER_VIEW_CONTEXT_MENU_MAC_REMOTE_COCOA_H_
