// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_

#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/views/toolbar/wallet_button_notification_source.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/interaction/element_identifier.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/menu_button_controller.h"

namespace brave_wallet {
FORWARD_DECLARE_TEST(WalletButtonButtonBrowserTest,
                     SizeIsNotEmptyWithAndWithoutBadge);
}  // namespace brave_wallet

class BrowserWindowInterface;
class PrefService;

class WalletButton : public ToolbarButton {
  METADATA_HEADER(WalletButton, ToolbarButton)

 public:
  DECLARE_CLASS_ELEMENT_IDENTIFIER_VALUE(kWalletButtonElementId);

  // Returns the wallet button of `browser`, or nullptr when its window has
  // none, such as popup and app windows. The button exists, even when hidden,
  // in normal windows.
  static WalletButton* From(BrowserWindowInterface* browser);

  WalletButton(View* backup_anchor_view, Profile* profile);
  ~WalletButton() override;

  WalletButton(const WalletButton&) = delete;
  WalletButton& operator=(const WalletButton&) = delete;

  void ShowWalletBubble();
  void ShowApproveWalletBubble();
  void CloseWalletBubble();
  bool IsShowingBubble();
  bool IsBubbleClosedForTesting();
  views::Widget* GetBubbleWidgetForTesting();

  void UpdateImageAndText();

  views::View* GetAsAnchorView();

 private:
  FRIEND_TEST_ALL_PREFIXES(brave_wallet::WalletButtonButtonBrowserTest,
                           SizeIsNotEmptyWithAndWithoutBadge);

  // views::View:
  void AddedToWidget() override;

  std::string GetBadgeText();
  void OnWalletPressed(const ui::Event& event);
  void OnNotificationUpdate(bool show_suggest_badge, size_t counter);

  // ToolbarButton overrides:
  void UpdateIcon() override;
  void OnThemeChanged() override;

  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<views::MenuButtonController> menu_button_controller_ = nullptr;
  raw_ptr<views::View> backup_anchor_view_ = nullptr;

  std::unique_ptr<brave_wallet::WalletButtonNotificationSource>
      notification_source_;

  size_t counter_ = 0;
  bool show_suggest_badge_ = false;

  base::WeakPtrFactory<WalletButton> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_
