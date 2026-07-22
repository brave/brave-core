/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_WALLET_WALLET_SIDE_PANEL_WEB_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_WALLET_WALLET_SIDE_PANEL_WEB_VIEW_H_

#include <memory>

#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"

class Profile;
class SidePanelEntryScope;
class WalletSidePanelContentsWrapper;

// Hosts WalletPanelUI in the side panel with auto-resize disabled so content
// fills the panel. Ownership of the contents wrapper lives on this view.
class WalletSidePanelWebView : public SidePanelWebUIView {
  METADATA_HEADER(WalletSidePanelWebView, SidePanelWebUIView)

 public:
  static std::unique_ptr<views::View> CreateView(Profile* profile,
                                                 SidePanelEntryScope& scope);

  WalletSidePanelWebView(
      SidePanelEntryScope& scope,
      std::unique_ptr<WalletSidePanelContentsWrapper> contents_wrapper);
  WalletSidePanelWebView(const WalletSidePanelWebView&) = delete;
  WalletSidePanelWebView& operator=(const WalletSidePanelWebView&) = delete;
  ~WalletSidePanelWebView() override;

 private:
  std::unique_ptr<WalletSidePanelContentsWrapper> contents_wrapper_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_WALLET_WALLET_SIDE_PANEL_WEB_VIEW_H_
