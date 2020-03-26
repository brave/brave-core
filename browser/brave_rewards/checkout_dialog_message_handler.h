/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_MESSAGE_HANDLER_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_MESSAGE_HANDLER_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_rewards/checkout_dialog_controller.h"
#include "brave/browser/brave_rewards/checkout_dialog_params.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace brave_rewards {

// Defines the interface between JS and C++ for the checkout dialog.
// This message handler is created by the checkout dialog delegate
// when the dialog is shown.
class CheckoutDialogMessageHandler
    : public content::WebUIMessageHandler,
      public RewardsServiceObserver,
      public CheckoutDialogController::Observer {

 public:
  CheckoutDialogMessageHandler(
      CheckoutDialogParams* params,
      CheckoutDialogController* controller);

  CheckoutDialogMessageHandler(
      const CheckoutDialogMessageHandler&) = delete;
  CheckoutDialogMessageHandler& operator=(
      const CheckoutDialogMessageHandler&) = delete;

  ~CheckoutDialogMessageHandler() override;

  // WebUIMessageHandler:
  void RegisterMessages() override;

  // RewardsServiceObserver:
  void OnWalletInitialized(RewardsService* service, int32_t result) override;
  void OnRewardsMainEnabled(RewardsService* service, bool enabled) override;

  // CheckoutDialogController::Observer:
  void OnPaymentAborted() override;
  void OnPaymentConfirmed() override;

 private:
  RewardsService* GetRewardsService();
  void FireServiceError(const std::string& type, int status);

  // Message handlers:
  void OnGetWalletBalance(const base::ListValue* args);
  void OnGetAnonWalletStatus(const base::ListValue* args);
  void OnGetExternalWallet(const base::ListValue* args);
  void OnGetRewardsParameters(const base::ListValue* args);
  void OnGetRewardsEnabled(const base::ListValue* args);
  void OnEnableRewards(const base::ListValue* args);
  void OnCreateWallet(const base::ListValue* args);
  void OnCancelPayment(const base::ListValue* args);
  void OnGetOrderInfo(const base::ListValue* args);
  void OnPayWithWallet(const base::ListValue* args);
  void OnPayWithCreditCard(const base::ListValue* args);

  // Rewards service callbacks:
  void FetchBalanceCallback(
      int32_t status,
      std::unique_ptr<Balance> balance);

  void GetExternalWalletCallback(
      int32_t status,
      std::unique_ptr<ExternalWallet> wallet);

  void GetRewardsParametersCallback(
      std::unique_ptr<RewardsParameters> parameters);

  void GetAnonWalletStatusCallback(uint32_t result);
  void GetRewardsMainEnabledCallback(bool enabled);
  void CreateWalletCallback(int32_t result);


  enum class PaymentState {
    None,
    Aborted,
    InProgress,
    Confirmed
  };

  PaymentState payment_state_ = PaymentState::None;
  CheckoutDialogParams* params_;  // Owned by CheckoutDialogDelegate
  CheckoutDialogController* controller_;  // Owned by CheckoutDialogDelegate
  RewardsService* rewards_service_ = nullptr;  // Immortal
  base::WeakPtrFactory<CheckoutDialogMessageHandler> weak_factory_;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_MESSAGE_HANDLER_H_
