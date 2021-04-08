/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_REWARDS_CHECKOUT_DIALOG_H_
#define BRAVE_BROWSER_UI_BRAVE_REWARDS_CHECKOUT_DIALOG_H_

#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace base {
class DictionaryValue;
}

namespace content {
class WebContents;
}

namespace payments {
class PaymentRequest;
}

namespace brave_rewards {

class RewardsService;

class CheckoutDialogDelegate : public ui::WebDialogDelegate {
 public:
  CheckoutDialogDelegate(base::Value params,
                         base::WeakPtr<payments::PaymentRequest> request);
  ~CheckoutDialogDelegate() override;

  ui::ModalType GetDialogModalType() const override;
  base::string16 GetDialogTitle() const override;
  GURL GetDialogContentURL() const override;
  void GetWebUIMessageHandlers(
      std::vector<content::WebUIMessageHandler*>* handlers) const override;
  void GetDialogSize(gfx::Size* size) const override;
  std::string GetDialogArgs() const override;
  void OnDialogClosed(const std::string& result) override;

  void OnCloseContents(content::WebContents* source,
                       bool* out_close_dialog) override;

  bool ShouldShowDialogTitle() const override;

 private:
  base::Value params_;
  base::WeakPtr<payments::PaymentRequest> request_;

  CheckoutDialogDelegate(const CheckoutDialogDelegate&) = delete;
  CheckoutDialogDelegate& operator=(const CheckoutDialogDelegate&) = delete;
};

class CheckoutDialogHandler : public content::WebUIMessageHandler {
 public:
  explicit CheckoutDialogHandler(
      base::WeakPtr<payments::PaymentRequest> request);
  ~CheckoutDialogHandler() override;

  // Overridden from WebUIMessageHandler
  void RegisterMessages() override;

 private:
  RewardsService* GetRewardsService();

  // Message handlers
  void HandlePaymentCompletion(const base::ListValue* args);
  void GetWalletBalance(const base::ListValue* args);
  void GetExternalWallet(const base::ListValue* args);
  void GetRewardsParameters(const base::ListValue* args);

  // Rewards service callbacks
  void OnFetchBalance(const ledger::mojom::Result result,
                      ledger::mojom::BalancePtr balance);
  void OnGetExternalWallet(const ledger::mojom::Result result,
                           ledger::mojom::ExternalWalletPtr wallet);
  void OnGetRewardsParameters(
      ledger::mojom::RewardsParametersPtr parameters);

  RewardsService* rewards_service_ = nullptr;  // NOT OWNED

  base::WeakPtr<payments::PaymentRequest> request_;
  std::vector<::ledger::mojom::SKUOrderItemPtr> items_;
  base::WeakPtrFactory<CheckoutDialogHandler> weak_factory_;

  CheckoutDialogHandler(const CheckoutDialogHandler&) = delete;
  CheckoutDialogHandler& operator=(const CheckoutDialogHandler&) = delete;
};

void ShowCheckoutDialog(base::WeakPtr<payments::PaymentRequest> request);

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_BRAVE_REWARDS_CHECKOUT_DIALOG_H_
