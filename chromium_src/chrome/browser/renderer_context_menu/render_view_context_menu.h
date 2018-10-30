// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_H_OVERRIDE_
#define CHROME_BROWSER_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_H_OVERRIDE_

// Get the Chromium declaration.
#define RenderViewContextMenu RenderViewContextMenu_Chromium
#include "../../../../../chrome/browser/renderer_context_menu/render_view_context_menu.h"
#undef RenderViewContextMenu

// Declare our own subclass with overridden methods.
class BraveRenderViewContextMenu : public RenderViewContextMenu_Chromium {
 public:
  BraveRenderViewContextMenu(content::RenderFrameHost* render_frame_host,
                             const content::ContextMenuParams& params);
  void AppendBraveLinkItems() override;
  // RenderViewContextMenuBase:
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int id, int event_flags) override;
  void AddSpellCheckServiceItem(bool is_checked) override;
  // Hide base class implementation.
  static void AddSpellCheckServiceItem(ui::SimpleMenuModel* menu,
                                       bool is_checked);
};

// Use our own subclass as the real RenderViewContextMenu.
#define RenderViewContextMenu BraveRenderViewContextMenu

#endif  // CHROME_BROWSER_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_H_OVERRIDE_
