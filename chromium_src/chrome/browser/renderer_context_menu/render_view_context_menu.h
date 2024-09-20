/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_H_

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

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
#define AppendReadingModeItem virtual AppendReadingModeItem
#define AppendDeveloperItems virtual AppendDeveloperItems
#include "src/chrome/browser/renderer_context_menu/render_view_context_menu.h"  // IWYU pragma: export
#undef AppendDeveloperItems
#undef AppendReadingModeItem
#undef RegisterMenuShownCallbackForTesting
#undef RenderViewContextMenu
#undef BRAVE_RENDER_VIEW_CONTEXT_MENU_H_

// Declare our own subclass with overridden methods.
class BraveRenderViewContextMenu : public RenderViewContextMenu_Chromium {
 public:
  // Non-const reference passed in the parent class upstream
  // NOLINTNEXTLINE(runtime/references)
  BraveRenderViewContextMenu(content::RenderFrameHost& render_frame_host,
                             const content::ContextMenuParams& params);
  ~BraveRenderViewContextMenu() override;
  // RenderViewContextMenuBase:
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int id, int event_flags) override;
  void AddSpellCheckServiceItem(bool is_checked) override;
  // Hide base class implementation.
  static void AddSpellCheckServiceItem(ui::SimpleMenuModel* menu,
                                       bool is_checked);
  void AddAccessibilityLabelsServiceItem(bool is_checked) override;
  // Do nothing as we have our own speed reader
  void AppendReadingModeItem() override {}

  void AppendDeveloperItems() override;

  void SetAIEngineForTesting(
      std::unique_ptr<ai_chat::EngineConsumer> ai_engine);
  ai_chat::EngineConsumer* GetAIEngineForTesting() { return ai_engine_.get(); }

 private:
  friend class BraveRenderViewContextMenuTest;
  // RenderViewContextMenuBase:
  void InitMenu() override;
  void NotifyMenuShown() override;

#if BUILDFLAG(ENABLE_AI_CHAT)
  bool IsAIChatEnabled() const;
  void ExecuteAIChatCommand(int command);
  void BuildAIChatMenu();
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
  void CopyTextFromImage();
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
  std::unique_ptr<ai_chat::EngineConsumer> ai_engine_;
  ui::SimpleMenuModel ai_chat_submenu_model_;
  ui::SimpleMenuModel ai_chat_change_tone_submenu_model_;
  ui::SimpleMenuModel ai_chat_change_length_submenu_model_;
  ui::SimpleMenuModel ai_chat_social_media_post_submenu_model_;
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
};

// Use our own subclass as the real RenderViewContextMenu.
#define RenderViewContextMenu BraveRenderViewContextMenu

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_RENDERER_CONTEXT_MENU_RENDER_VIEW_CONTEXT_MENU_H_
