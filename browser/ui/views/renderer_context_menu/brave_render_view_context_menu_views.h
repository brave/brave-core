/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_RENDERER_CONTEXT_MENU_BRAVE_RENDER_VIEW_CONTEXT_MENU_VIEWS_H_
#define BRAVE_BROWSER_UI_VIEWS_RENDERER_CONTEXT_MENU_BRAVE_RENDER_VIEW_CONTEXT_MENU_VIEWS_H_

#include "chrome/browser/ui/views/renderer_context_menu/render_view_context_menu_views.h"

class BraveRenderViewContextMenuViews : public RenderViewContextMenuViews {
 public:
  ~BraveRenderViewContextMenuViews() override;
  BraveRenderViewContextMenuViews(const BraveRenderViewContextMenuViews&) =
      delete;
  BraveRenderViewContextMenuViews& operator=(
      const BraveRenderViewContextMenuViews&) = delete;

  // Factory function to create an instance.
  static RenderViewContextMenuViews* Create(
      content::RenderFrameHost* render_frame_host,
      const content::ContextMenuParams& params);

  void Show() override;

 protected:
  BraveRenderViewContextMenuViews(content::RenderFrameHost* render_frame_host,
                                  const content::ContextMenuParams& params);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_RENDERER_CONTEXT_MENU_BRAVE_RENDER_VIEW_CONTEXT_MENU_VIEWS_H_
