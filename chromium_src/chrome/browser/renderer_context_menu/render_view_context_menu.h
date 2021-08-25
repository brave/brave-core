// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_H_

#include "brave/components/ipfs/buildflags/buildflags.h"

#define BRAVE_RENDER_VIEW_CONTEXT_MENU_H_ \
  private: \
    friend class BraveRenderViewContextMenu; \
  public:
// define BRAVE_RENDER_VIEW_CONTEXT_MENU_H_

// Get the Chromium declaration.
#define RenderViewContextMenu RenderViewContextMenu_Chromium

class BraveRenderViewContextMenu;

#define RegisterMenuShownCallbackForTesting                      \
  RegisterMenuShownCallbackForTesting(                           \
      base::OnceCallback<void(BraveRenderViewContextMenu*)> cb); \
  static void RegisterMenuShownCallbackForTesting_unused
#include "../../../../../chrome/browser/renderer_context_menu/render_view_context_menu.h"
#undef RegisterMenuShownCallbackForTesting
#undef RenderViewContextMenu

// Declare our own subclass with overridden methods.
class BraveRenderViewContextMenu : public RenderViewContextMenu_Chromium {
 public:
  BraveRenderViewContextMenu(content::RenderFrameHost* render_frame_host,
                             const content::ContextMenuParams& params);
  // RenderViewContextMenuBase:
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int id, int event_flags) override;
  void AddSpellCheckServiceItem(bool is_checked) override;
  // Hide base class implementation.
  static void AddSpellCheckServiceItem(ui::SimpleMenuModel* menu,
                                       bool is_checked);

 private:
  // RenderViewContextMenuBase:
  void InitMenu() override;
#if BUILDFLAG(ENABLE_IPFS)
  void SeIpfsIconAt(int index);
  void BuildIPFSMenu();
  void ExecuteIPFSCommand(int id, int event_flags);
  bool IsIPFSCommandIdEnabled(int command) const;

  ui::SimpleMenuModel ipfs_submenu_model_;
#endif
};

// Use our own subclass as the real RenderViewContextMenu.
#define RenderViewContextMenu BraveRenderViewContextMenu

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_H_
