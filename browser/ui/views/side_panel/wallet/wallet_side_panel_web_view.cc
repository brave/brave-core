/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/wallet/wallet_side_panel_web_view.h"

#include <utility>

#include "base/check.h"
#include "base/functional/callback.h"
#include "brave/browser/ui/views/side_panel/wallet/wallet_side_panel_contents_wrapper.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "url/gurl.h"

BEGIN_METADATA(WalletSidePanelWebView)
END_METADATA

// static
std::unique_ptr<views::View> WalletSidePanelWebView::CreateView(
    Profile* profile,
    SidePanelEntryScope& scope) {
  CHECK(profile);

  auto contents_wrapper = std::make_unique<WalletSidePanelContentsWrapper>(
      GURL(kBraveUIWalletPanelURL), profile, IDS_SIDEBAR_WALLET_ITEM_TITLE,
      /*esc_closes_ui=*/false);

  auto web_view = std::make_unique<WalletSidePanelWebView>(
      scope, std::move(contents_wrapper));
  web_view->ShowUI();
  return web_view;
}

WalletSidePanelWebView::WalletSidePanelWebView(
    SidePanelEntryScope& scope,
    std::unique_ptr<WalletSidePanelContentsWrapper> contents_wrapper)
    : SidePanelWebUIView(scope,
                         /*on_show_cb=*/base::RepeatingClosure(),
                         /*close_cb=*/base::RepeatingClosure(),
                         contents_wrapper.get()),
      contents_wrapper_(std::move(contents_wrapper)) {}

WalletSidePanelWebView::~WalletSidePanelWebView() = default;
