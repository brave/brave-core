/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PANEL_WEBVIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PANEL_WEBVIEW_H_

#include <memory>

#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"
#include "ui/views/controls/webview/webview.h"

namespace ui {
class MenuModel;
}  // namespace ui

namespace views {
class MenuRunner;
}  // namespace views

class SidebarPanelWebView : public views::WebView {
 public:
  METADATA_HEADER(SidebarPanelWebView);
  explicit SidebarPanelWebView(content::BrowserContext* browser_context);
  ~SidebarPanelWebView() override;
  SidebarPanelWebView(const SidebarPanelWebView&) = delete;
  SidebarPanelWebView& operator=(const SidebarPanelWebView&) = delete;

  void ShowCustomContextMenu(const gfx::Point& point,
                             std::unique_ptr<ui::MenuModel> menu_model);
  void HideCustomContextMenu();
  bool TreatUnHandledKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event);

 private:
  std::unique_ptr<views::MenuRunner> context_menu_runner_;
  std::unique_ptr<ui::MenuModel> context_menu_model_;
  views::UnhandledKeyboardEventHandler unhandled_keyboard_event_handler_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PANEL_WEBVIEW_H_
