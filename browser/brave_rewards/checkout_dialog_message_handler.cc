/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/checkout_dialog_message_handler.h"

#include <string>
#include <utility>

#include "base/time/time.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "chrome/browser/profiles/profile.h"

namespace brave_rewards {

using base::Bind;
using base::BindOnce;
using base::BindRepeating;

CheckoutDialogMessageHandler::CheckoutDialogMessageHandler(
    CheckoutDialogParams* params,
    CheckoutDialogController* controller)
    : params_(params),
      controller_(controller),
      weak_factory_(this) {
  DCHECK(params_);
  DCHECK(controller_);
  controller_->AddObserver(this);
}

CheckoutDialogMessageHandler::~CheckoutDialogMessageHandler() {
  controller_->RemoveObserver(this);
  if (rewards_service_) {
    rewards_service_->RemoveObserver(this);
  }
}

RewardsService* CheckoutDialogMessageHandler::GetRewardsService() {
  if (!rewards_service_) {
    Profile* profile = Profile::FromWebUI(web_ui());
    rewards_service_ = RewardsServiceFactory::GetForProfile(profile);
    if (rewards_service_) {
      rewards_service_->AddObserver(this);
    }
  }
  return rewards_service_;
}

void CheckoutDialogMessageHandler::FireServiceError(
    const std::string& type,
    int status) {
  base::Value response(base::Value::Type::DICTIONARY);
  response.SetStringKey("type", type);
  response.SetIntKey("status", status);
  FireWebUIListener("serviceError", response);
}

void CheckoutDialogMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("getWalletBalance", BindRepeating(
      &CheckoutDialogMessageHandler::OnGetWalletBalance,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("getAnonWalletStatus", BindRepeating(
      &CheckoutDialogMessageHandler::OnGetAnonWalletStatus,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("getExternalWallet", BindRepeating(
      &CheckoutDialogMessageHandler::OnGetExternalWallet,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("getRewardsParameters", BindRepeating(
      &CheckoutDialogMessageHandler::OnGetRewardsParameters,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("getRewardsEnabled", BindRepeating(
      &CheckoutDialogMessageHandler::OnGetRewardsEnabled,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("enableRewards", BindRepeating(
      &CheckoutDialogMessageHandler::OnEnableRewards,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("createWallet", BindRepeating(
      &CheckoutDialogMessageHandler::OnCreateWallet,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("cancelPayment", BindRepeating(
      &CheckoutDialogMessageHandler::OnCancelPayment,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("getOrderInfo", BindRepeating(
      &CheckoutDialogMessageHandler::OnGetOrderInfo,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("payWithCreditCard", BindRepeating(
      &CheckoutDialogMessageHandler::OnPayWithCreditCard,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("payWithWallet", BindRepeating(
      &CheckoutDialogMessageHandler::OnPayWithWallet,
      base::Unretained(this)));
}

void CheckoutDialogMessageHandler::OnWalletInitialized(
    RewardsService* service,
    int32_t result) {
  if (IsJavascriptAllowed()) {
    base::Value response(base::Value::Type::DICTIONARY);
    response.SetIntKey("status", result);
    FireWebUIListener("walletInitialized", std::move(response));
  }
}

void CheckoutDialogMessageHandler::OnRewardsMainEnabled(
    RewardsService* service,
    bool enabled) {
  GetRewardsMainEnabledCallback(enabled);
}

void CheckoutDialogMessageHandler::OnPaymentAborted() {
  if (payment_state_ == PaymentState::None) {
    payment_state_ = PaymentState::Aborted;
    if (IsJavascriptAllowed()) {
      FireWebUIListener("dialogDismissed");
    }
  }
}

void CheckoutDialogMessageHandler::OnPaymentConfirmed() {
  payment_state_ = PaymentState::Confirmed;
  if (IsJavascriptAllowed()) {
    FireWebUIListener("paymentConfirmed");
  }
}

void CheckoutDialogMessageHandler::OnGetWalletBalance(
    const base::ListValue* args) {
  if (Profile::FromWebUI(web_ui())->IsOffTheRecord()) {
    AllowJavascript();
    FireServiceError("off-the-record", 0);
    return;
  }
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->FetchBalance(BindOnce(
        &CheckoutDialogMessageHandler::FetchBalanceCallback,
        weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogMessageHandler::OnGetAnonWalletStatus(
    const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->GetAnonWalletStatus(BindOnce(
        &CheckoutDialogMessageHandler::GetAnonWalletStatusCallback,
        weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogMessageHandler::OnGetExternalWallet(
    const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->GetExternalWallet("uphold", BindOnce(
        &CheckoutDialogMessageHandler::GetExternalWalletCallback,
        weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogMessageHandler::OnGetRewardsParameters(
    const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->GetRewardsParameters(BindOnce(
        &CheckoutDialogMessageHandler::GetRewardsParametersCallback,
        weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogMessageHandler::OnGetRewardsEnabled(
    const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->GetRewardsMainEnabled(Bind(
        &CheckoutDialogMessageHandler::GetRewardsMainEnabledCallback,
        weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogMessageHandler::OnEnableRewards(
    const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->SetRewardsMainEnabled(1);
  }
}

void CheckoutDialogMessageHandler::OnCreateWallet(
    const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->CreateWallet(Bind(
        &CheckoutDialogMessageHandler::CreateWalletCallback,
        weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogMessageHandler::OnCancelPayment(
    const base::ListValue* args) {
  if (payment_state_ != PaymentState::InProgress) {
    AllowJavascript();
    FireWebUIListener("dialogDismissed");
  }
}

void CheckoutDialogMessageHandler::OnGetOrderInfo(
    const base::ListValue* args) {
  AllowJavascript();
  base::Value order_info(base::Value::Type::DICTIONARY);
  order_info.SetStringKey("description", params_->description);
  order_info.SetDoubleKey("total", params_->total);
  order_info.SetBoolKey("aborted", payment_state_ == PaymentState::Aborted);
  FireWebUIListener("orderInfoUpdated", order_info);
}

void CheckoutDialogMessageHandler::OnPayWithWallet(
    const base::ListValue* args) {
  DCHECK(payment_state_ == PaymentState::None);
  payment_state_ = PaymentState::InProgress;

  // TODO(zenparsing): Call GetRewardsService()->ProcessSKU,
  // providing a vector of SKUOrderItems. The rewards service
  // currently uses only the "sku" and "quantity" fields. The
  // ProcessSKU method also requires an uphold wallet ptr,
  // which is problematic for this UI. We also should pass in
  // the total that was displayed to the user so that we don't
  // inadvertantly charge them the incorrect amount.

  // TODO(zenparsing): ProcessSKU returns an SKUOrder pointer. Do we
  // need to check the "status" for FULFILLED?
  std::string order_id = "temp_order_id";
  controller_->NotifyPaymentReady(order_id);
}

void CheckoutDialogMessageHandler::OnPayWithCreditCard(
    const base::ListValue* args) {
  // NOTE: Implementation required for credit card integration
  NOTREACHED();
}

void CheckoutDialogMessageHandler::FetchBalanceCallback(
    int32_t status,
    std::unique_ptr<Balance> balance) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  if (status == 0 && balance) {
    base::Value response(base::Value::Type::DICTIONARY);
    response.SetDoubleKey("total", balance->total);
    FireWebUIListener("walletBalanceUpdated", response);
  } else {
    FireServiceError("fetch-balance-error", status);
  }
}

void CheckoutDialogMessageHandler::GetAnonWalletStatusCallback(
    uint32_t status) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  std::string status_text;
  switch (status) {
    case 12: status_text = "created"; break;
    case 17: status_text = "corrupted"; break;
    default: status_text = "not-created"; break;
  }

  base::Value response(base::Value::Type::DICTIONARY);
  response.SetStringKey("status", status_text);
  FireWebUIListener("anonWalletStatusUpdated", response);
}

void CheckoutDialogMessageHandler::GetExternalWalletCallback(
    int32_t status,
    std::unique_ptr<ExternalWallet> wallet) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  if (status == 0 && wallet) {
    bool verified = wallet->status == 1;  // ledger::WalletStatus::VERIFIED

    base::Value response(base::Value::Type::DICTIONARY);
    response.SetBoolKey("verified", verified);

    FireWebUIListener("externalWalletUpdated", response);
  } else {
    FireServiceError("get-external-wallet-error", status);
  }
}

void CheckoutDialogMessageHandler::GetRewardsParametersCallback(
    std::unique_ptr<RewardsParameters> parameters) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  if (parameters) {
    base::Value response(base::Value::Type::DICTIONARY);
    response.SetDoubleKey("rate", parameters->rate);
    response.SetDoubleKey(
        "lastUpdated",
        base::Time::Now().ToJsTimeIgnoringNull());
    FireWebUIListener("rewardsParametersUpdated", response);
  } else {
    FireServiceError("get-rewards-parameters-error", 0);
  }
}

void CheckoutDialogMessageHandler::GetRewardsMainEnabledCallback(
    bool enabled) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value response(base::Value::Type::DICTIONARY);
  response.SetBoolKey("rewardsEnabled", enabled);

  FireWebUIListener("rewardsEnabledUpdated", response);
}

void CheckoutDialogMessageHandler::CreateWalletCallback(int32_t result) {
  // JS will be informed of the wallet creation result via
  // the WalletInitialized callback.
}

}  // namespace brave_rewards
