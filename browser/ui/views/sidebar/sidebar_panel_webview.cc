/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_panel_webview.h"

#include <utility>

#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/controls/menu/menu_runner.h"

SidebarPanelWebView::SidebarPanelWebView(
    content::BrowserContext* browser_context)
    : WebView(browser_context) {
  SetVisible(false);
  set_allow_accelerators(true);
}

SidebarPanelWebView::~SidebarPanelWebView() = default;

void SidebarPanelWebView::ShowCustomContextMenu(
    const gfx::Point& point,
    std::unique_ptr<ui::MenuModel> menu_model) {
  // Show context menu at in screen coordinates.
  gfx::Point screen_point = point;
  ConvertPointToScreen(this, &screen_point);
  context_menu_model_ = std::move(menu_model);
  context_menu_runner_ = std::make_unique<views::MenuRunner>(
      context_menu_model_.get(),
      views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU);
  context_menu_runner_->RunMenuAt(
      GetWidget(), nullptr, gfx::Rect(screen_point, gfx::Size()),
      views::MenuAnchorPosition::kTopLeft, ui::MENU_SOURCE_MOUSE,
      web_contents()->GetContentNativeView());
}

void SidebarPanelWebView::HideCustomContextMenu() {
  if (context_menu_runner_)
    context_menu_runner_->Cancel();
}

bool SidebarPanelWebView::TreatUnHandledKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  return unhandled_keyboard_event_handler_.HandleKeyboardEvent(
      event, GetFocusManager());
}

BEGIN_METADATA(SidebarPanelWebView, views::WebView)
END_METADATA
