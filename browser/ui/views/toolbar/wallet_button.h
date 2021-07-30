// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_

#include <memory>

#include "base/time/time.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "components/prefs/pref_change_registrar.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/menu_button_controller.h"
#include "ui/views/widget/widget_observer.h"

class PrefService;
class Profile;

class WalletButton : public ToolbarButton, public views::WidgetObserver {
  METADATA_HEADER(WalletButton);

 public:
  WalletButton(View* backup_anchor_view, Profile* profile, PrefService* prefs);
  ~WalletButton() override;

  WalletButton(const WalletButton&) = delete;
  WalletButton& operator=(const WalletButton&) = delete;

  // When this is called the bubble may already be showing or be loading in.
  // This returns true if the method call results in the creation of a new Tab
  // Search bubble.
  bool ShowWalletBubble();
  void CloseWalletBubble();

  void UpdateImageAndText();
  void InitBubbleManagerAnchor();
  void UpdateVisibility();

  WebUIBubbleManager* webui_bubble_manager_for_testing() {
    return webui_bubble_manager_.get();
  }

 private:
  void OnPreferenceChanged();
  void OnWalletPressed(const ui::Event& event);

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  std::unique_ptr<WebUIBubbleManagerT<WalletPanelUI>> webui_bubble_manager_;

  PrefService* prefs_ = nullptr;
  View* backup_anchor_view_ = nullptr;
  Profile* profile_;
  PrefChangeRegistrar pref_change_registrar_;

  views::MenuButtonController* menu_button_controller_ = nullptr;

  // A lock to keep the button pressed while the bubble is showing or
  // in the process of being shown.
  std::unique_ptr<views::MenuButtonController::PressedLock> pressed_lock_;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      bubble_widget_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_
