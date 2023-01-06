// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "brave/browser/ui/views/toolbar/wallet_button_notification_source.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/menu_button_controller.h"

class PrefService;

class WalletButton : public ToolbarButton {
  METADATA_HEADER(WalletButton);

 public:
  WalletButton(View* backup_anchor_view, Profile* profile);
  ~WalletButton() override;

  WalletButton(const WalletButton&) = delete;
  WalletButton& operator=(const WalletButton&) = delete;

  void ShowWalletBubble();
  void ShowApproveWalletBubble();
  void CloseWalletBubble();
  bool IsShowingBubble();
  bool IsBubbleClosedForTesting();

  void UpdateImageAndText();
  void UpdateVisibility();

  views::View* GetAsAnchorView();

 private:
  void AddedToWidget() override;
  std::string GetBadgeText();
  void OnPreferenceChanged();
  void OnWalletPressed(const ui::Event& event);
  void OnNotificationUpdate(bool show_suggest_badge, size_t counter);

  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<views::MenuButtonController> menu_button_controller_ = nullptr;
  raw_ptr<views::View> backup_anchor_view_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;

  std::unique_ptr<brave::WalletButtonNotificationSource> notification_source_;

  size_t counter_ = 0;
  bool show_suggest_badge_ = false;

  base::WeakPtrFactory<WalletButton> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_
