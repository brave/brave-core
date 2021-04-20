/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_rewards/checkout_dialog.h"

#include <memory>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/common/constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/payments/content/payment_request.h"
#include "components/payments/core/payer_data.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/mojom/payments/payment_request.mojom.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

using content::WebContents;
using content::WebUIMessageHandler;
using payments::PaymentRequest;
using payments::mojom::PaymentErrorReason;

namespace brave_rewards {

namespace {

constexpr int kDialogWidth = 548;
constexpr int kDialogMinHeight = 200;
constexpr int kDialogMaxHeight = 800;

void OnSKUProcessed(base::WeakPtr<payments::PaymentRequest> request,
                    const ledger::mojom::Result result,
                    const std::string& value) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!request) {
    return;
  }

  if (result == ledger::mojom::Result::LEDGER_OK) {
    payments::mojom::PaymentDetailsPtr details =
        payments::mojom::PaymentDetails::New();
    details->id = value;
    request->spec()->UpdateWith(std::move(details));
    request->Pay();
    return;
  }
  request->OnError(PaymentErrorReason::UNKNOWN, errors::kBatTransactionFailed);
}

void OnGetPublisherDetailsCallback(base::WeakPtr<PaymentRequest> request,
                                   const ledger::mojom::Result result,
                                   ledger::mojom::PublisherInfoPtr info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  double total = 0;
  if (!request) {
    return;
  }

  if (!info || info->status == ledger::mojom::PublisherStatus::NOT_VERIFIED) {
    request->OnError(PaymentErrorReason::NOT_SUPPORTED,
                     brave_rewards::errors::kInvalidPublisher);
    return;
  }

  base::WeakPtr<payments::PaymentRequestSpec> spec = request->spec();
  if (!spec) {
    request->OnError(PaymentErrorReason::INVALID_DATA_FROM_RENDERER,
                     errors::kInvalidData);
    return;
  }
  auto* contents = request->web_contents();

  DCHECK(base::StringToDouble(spec->details().total->amount->value, &total));
  base::Value order_info(base::Value::Type::DICTIONARY);
  order_info.SetDoubleKey("total", total);

  base::Value params(base::Value::Type::DICTIONARY);
  params.SetKey("orderInfo", std::move(order_info));

  ShowConstrainedWebDialogWithAutoResize(
      contents->GetBrowserContext(),
      std::make_unique<CheckoutDialogDelegate>(std::move(params), request),
      contents, gfx::Size(kDialogWidth, kDialogMinHeight),
      gfx::Size(kDialogWidth, kDialogMaxHeight));
}

}  // namespace

enum DialogCloseReason {
  Complete,
  InsufficientBalance,
  UnverifiedWallet,
  UserCancelled
};

CheckoutDialogDelegate::CheckoutDialogDelegate(
    base::Value params,
    base::WeakPtr<PaymentRequest> request)
    : params_(std::move(params)), request_(request) {}

CheckoutDialogDelegate::~CheckoutDialogDelegate() = default;

ui::ModalType CheckoutDialogDelegate::GetDialogModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 CheckoutDialogDelegate::GetDialogTitle() const {
  return base::string16();
}

GURL CheckoutDialogDelegate::GetDialogContentURL() const {
  return GURL(kBraveUICheckoutURL);
}

void CheckoutDialogDelegate::GetWebUIMessageHandlers(
    std::vector<WebUIMessageHandler*>* handlers) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!request_) {
    return;
  }
  handlers->push_back(new CheckoutDialogHandler(request_));
}

void CheckoutDialogDelegate::GetDialogSize(gfx::Size* size) const {}

std::string CheckoutDialogDelegate::GetDialogArgs() const {
  std::string json;
  base::JSONWriter::Write(params_, &json);
  return json;
}

void CheckoutDialogDelegate::OnDialogClosed(const std::string& result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  int reason = 0;
  DCHECK(base::StringToInt(result, &reason));

  if (!request_) {
    return;
  }

  switch (reason) {
    case DialogCloseReason::UserCancelled:
      request_->OnError(PaymentErrorReason::USER_CANCEL,
                        errors::kTransactionCancelled);
      break;
    case DialogCloseReason::UnverifiedWallet:
      request_->OnError(PaymentErrorReason::NOT_SUPPORTED,
                        errors::kUnverifiedUserWallet);
      break;
    case DialogCloseReason::InsufficientBalance:
      request_->OnError(PaymentErrorReason::NOT_SUPPORTED,
                        errors::kInsufficientBalance);
      break;
  }
}

void CheckoutDialogDelegate::OnCloseContents(WebContents* source,
                                             bool* out_close_dialog) {
  *out_close_dialog = true;
}

bool CheckoutDialogDelegate::ShouldShowDialogTitle() const {
  return false;
}

CheckoutDialogHandler::CheckoutDialogHandler(
    base::WeakPtr<PaymentRequest> request)
    : request_(request), weak_factory_(this) {}

CheckoutDialogHandler::~CheckoutDialogHandler() = default;

RewardsService* CheckoutDialogHandler::GetRewardsService() {
  if (!rewards_service_) {
    Profile* profile = Profile::FromWebUI(web_ui());
    rewards_service_ = RewardsServiceFactory::GetForProfile(profile);
  }
  return rewards_service_;
}

void CheckoutDialogHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "paymentRequestComplete",
      base::BindRepeating(&CheckoutDialogHandler::HandlePaymentCompletion,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getWalletBalance",
      base::BindRepeating(&CheckoutDialogHandler::GetWalletBalance,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getExternalWallet",
      base::BindRepeating(&CheckoutDialogHandler::GetExternalWallet,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getRewardsParameters",
      base::BindRepeating(&CheckoutDialogHandler::GetRewardsParameters,
                          base::Unretained(this)));
}

void CheckoutDialogHandler::HandlePaymentCompletion(
    const base::ListValue* args) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!request_) {
    return;
  }

  auto spec = request_->spec();
  if (!request_->spec()->IsInitialized()) {
    return;
  }

  auto* rewards_service = GetRewardsService();
  if (!rewards_service) {
    request_->OnError(PaymentErrorReason::INVALID_DATA_FROM_RENDERER,
                      errors::kRewardsNotInitialized);
    return;
  }

  const auto& display_items =
      spec->GetDisplayItems(request_->state()->selected_app());
  for (size_t i = 0; i < display_items.size(); i++) {
    DCHECK((*display_items[i])->sku.has_value());
    auto item = ledger::mojom::SKUOrderItem::New();

    item->sku = (*display_items[i])->sku.value();
    item->quantity = 1;
    items_.push_back(std::move(item));
  }

  auto callback = base::BindOnce(&OnSKUProcessed, request_);

  rewards_service->ProcessSKU(
      std::move(items_), ledger::constant::kWalletUphold, std::move(callback));
}

void CheckoutDialogHandler::GetRewardsParameters(const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->GetRewardsParameters(
        base::Bind(&CheckoutDialogHandler::OnGetRewardsParameters,
                   weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogHandler::OnGetRewardsParameters(
    ledger::mojom::RewardsParametersPtr parameters) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);
  if (parameters) {
    data.SetDoubleKey("rate", parameters->rate);
    data.SetDoubleKey("lastUpdated", base::Time::Now().ToJsTimeIgnoringNull());
  }
  FireWebUIListener("rewardsParametersUpdated", data);
}

void CheckoutDialogHandler::GetWalletBalance(const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->FetchBalance(
        base::BindOnce(&CheckoutDialogHandler::OnFetchBalance,
                       weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogHandler::OnFetchBalance(
    const ledger::mojom::Result result,
    ledger::mojom::BalancePtr balance) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);
  data.SetDoubleKey("total", balance->total);

  FireWebUIListener("walletBalanceUpdated", data);
}

void CheckoutDialogHandler::GetExternalWallet(const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->GetExternalWallet(
        base::BindOnce(&CheckoutDialogHandler::OnGetExternalWallet,
                       weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogHandler::OnGetExternalWallet(
    const ledger::mojom::Result result,
    ledger::mojom::ExternalWalletPtr wallet) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);

  if (wallet) {
    data.SetIntKey("status", static_cast<int>(wallet->status));
  }
  FireWebUIListener("externalWalletUpdated", data);
}

void ShowCheckoutDialog(base::WeakPtr<PaymentRequest> request) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!request) {
    return;
  }

  WebContents* initiator = request->web_contents();
  Profile* profile =
      Profile::FromBrowserContext(initiator->GetBrowserContext());

  // BAT payment method doesn't work in private mode
  if (profile->IsOffTheRecord()) {
    request->OnError(PaymentErrorReason::NOT_SUPPORTED,
                     brave_rewards::errors::kBraveRewardsNotEnabled);
    return;
  }

  // BAT payment method only works for verified publishers
  auto* service = brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  if (service && service->IsRewardsEnabled()) {
    service->GetPublisherInfo(
        initiator->GetLastCommittedURL().GetOrigin().host(),
        base::BindOnce(&OnGetPublisherDetailsCallback, request));
  } else {
    request->OnError(PaymentErrorReason::NOT_SUPPORTED,
                     brave_rewards::errors::kBraveRewardsNotEnabled);
  }
}

}  // namespace brave_rewards
