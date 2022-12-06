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
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/browser/tx_status_resolver.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "components/prefs/pref_change_registrar.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/menu_button_controller.h"

class PrefService;

class WalletButton : public ToolbarButton,
                     brave_wallet::mojom::TxServiceObserver {
  METADATA_HEADER(WalletButton);

 public:
  WalletButton(View* backup_anchor_view,
               PrefService* prefs,
               content::BrowserContext* browser_context);
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

  void OnNewUnapprovedTx(
      brave_wallet::mojom::TransactionInfoPtr tx_info) override;

  void OnUnapprovedTxUpdated(
      brave_wallet::mojom::TransactionInfoPtr tx_info) override;

  void OnTransactionStatusChanged(
      brave_wallet::mojom::TransactionInfoPtr tx_info) override;

  void OnTxServiceReset() override;

 private:
  std::pair<std::string, SkColor> GetBadgeTextAndBackground();
  void CheckTxStatus();
  void OnPreferenceChanged();
  void OnWalletPressed(const ui::Event& event);
  void OnTxStatusResolved(size_t count);

  PrefChangeRegistrar pref_change_registrar_;
  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<brave_wallet::TxService> tx_service_;
  raw_ptr<views::MenuButtonController> menu_button_controller_ = nullptr;
  raw_ptr<views::View> backup_anchor_view_ = nullptr;
  mojo::Receiver<brave_wallet::mojom::TxServiceObserver> tx_observer_{this};
  std::unique_ptr<brave_wallet::TxStatusResolver> status_resolver_;
  size_t running_tx_count_ = 0;
  base::WeakPtrFactory<WalletButton> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_
