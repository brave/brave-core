/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_origin_startup/brave_origin_startup_handler.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_service.h"

namespace {

constexpr char kRemainingCredentialCount[] = "remaining_credential_count";
constexpr char kExpiresAt[] = "expires_at";

bool ParseCredentialSummary(const std::string& message) {
  std::string trimmed;
  base::TrimWhitespaceASCII(message, base::TrimPositions::TRIM_ALL, &trimmed);
  if (trimmed.empty()) {
    return false;
  }

  std::optional<base::DictValue> records = base::JSONReader::ReadDict(
      message, base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records || records->empty()) {
    return false;
  }

  int remaining = records->FindInt(kRemainingCredentialCount).value_or(0);
  const std::string* expires_at = records->FindString(kExpiresAt);
  return remaining > 0 || (expires_at != nullptr && !expires_at->empty());
}

}  // namespace

BraveOriginStartupHandler::BraveOriginStartupHandler(
    SkusServiceGetter skus_service_getter,
    PrefService* local_state,
    OpenBuyWindowCallback open_buy_window_callback,
    CloseDialogCallback close_dialog_callback)
    : open_buy_window_callback_(std::move(open_buy_window_callback)),
      close_dialog_callback_(std::move(close_dialog_callback)),
      // STAGING for unofficial builds; official builds always resolve to prod.
      origin_sku_domain_(brave_domains::GetServicesDomain(
          "origin",
          brave_domains::ServicesEnvironment::STAGING)),
      skus_service_getter_(std::move(skus_service_getter)),
      local_state_(local_state) {
  CHECK(local_state_);
}

BraveOriginStartupHandler::~BraveOriginStartupHandler() = default;

void BraveOriginStartupHandler::BindInterface(
    mojo::PendingReceiver<brave_origin::mojom::BraveOriginStartupHandler>
        receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void BraveOriginStartupHandler::CheckPurchaseState(
    CheckPurchaseStateCallback callback) {
  if (!EnsureSkusConnected()) {
    std::move(callback).Run(false);
    return;
  }

  skus_service_->CredentialSummary(
      origin_sku_domain_,
      base::BindOnce(&BraveOriginStartupHandler::OnCredentialSummary,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveOriginStartupHandler::VerifyPurchaseId(
    const std::string& purchase_id,
    VerifyPurchaseIdCallback callback) {
  if (!EnsureSkusConnected()) {
    std::move(callback).Run(false, "Unable to connect to SKU service");
    return;
  }

  // Trim whitespace from the purchase ID.
  std::string trimmed_id;
  base::TrimWhitespaceASCII(purchase_id, base::TrimPositions::TRIM_ALL,
                            &trimmed_id);
  if (trimmed_id.empty()) {
    std::move(callback).Run(false, "Purchase ID is empty");
    return;
  }

  VLOG(1) << "VerifyPurchaseId: domain=" << origin_sku_domain_
          << ", env=" << skus::GetEnvironmentForDomain(origin_sku_domain_);

  // The "purchase ID" from the email is actually the SKU order ID.
  // RefreshOrder fetches it from the server and stores it locally.
  skus_service_->RefreshOrder(
      origin_sku_domain_, trimmed_id,
      base::BindOnce(&BraveOriginStartupHandler::OnRefreshOrder,
                     weak_ptr_factory_.GetWeakPtr(), trimmed_id,
                     std::move(callback)));
}

void BraveOriginStartupHandler::OpenBuyWindow() {
  if (open_buy_window_callback_) {
    open_buy_window_callback_.Run();
  }
}

void BraveOriginStartupHandler::CloseDialog() {
  if (!local_state_ ||
      !local_state_->GetBoolean(brave_origin::kOriginPurchaseValidated)) {
    return;
  }

  if (close_dialog_callback_) {
    std::move(close_dialog_callback_).Run();
  }
}

void BraveOriginStartupHandler::ProceedFree() {
#if BUILDFLAG(IS_LINUX)
  if (local_state_) {
    local_state_->SetBoolean(brave_origin::kOriginFreeTierAccepted, true);
  }
  if (close_dialog_callback_) {
    std::move(close_dialog_callback_).Run();
  }
#endif
}

bool BraveOriginStartupHandler::EnsureSkusConnected() {
  if (!skus_service_) {
    if (skus_service_getter_) {
      auto pending = skus_service_getter_.Run();
      if (pending.is_valid()) {
        skus_service_.Bind(std::move(pending));
        skus_service_.reset_on_disconnect();
      }
    }
  }
  return !!skus_service_;
}

void BraveOriginStartupHandler::OnCredentialSummary(
    CheckPurchaseStateCallback callback,
    skus::mojom::SkusResultPtr summary) {
  bool purchased = ParseCredentialSummary(summary->message);

  if (purchased && local_state_) {
    local_state_->SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  }

  std::move(callback).Run(purchased);
}

void BraveOriginStartupHandler::OnRefreshOrder(
    const std::string& order_id,
    VerifyPurchaseIdCallback callback,
    skus::mojom::SkusResultPtr result) {
  if (result->code != skus::mojom::SkusResultCode::Ok) {
    VLOG(1) << "RefreshOrder failed with code: "
            << static_cast<int>(result->code);
    std::move(callback).Run(false, "Invalid purchase ID");
    return;
  }

  skus_service_->FetchOrderCredentials(
      origin_sku_domain_, order_id,
      base::BindOnce(&BraveOriginStartupHandler::OnFetchOrderCredentials,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveOriginStartupHandler::OnFetchOrderCredentials(
    VerifyPurchaseIdCallback callback,
    skus::mojom::SkusResultPtr result) {
  if (result->code != skus::mojom::SkusResultCode::Ok) {
    VLOG(1) << "FetchOrderCredentials failed with code: "
            << static_cast<int>(result->code);
    std::move(callback).Run(false, "Failed to fetch credentials");
    return;
  }

  // Confirm via CredentialSummary
  skus_service_->CredentialSummary(
      origin_sku_domain_,
      base::BindOnce(&BraveOriginStartupHandler::OnVerifyCredentialSummary,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveOriginStartupHandler::OnVerifyCredentialSummary(
    VerifyPurchaseIdCallback callback,
    skus::mojom::SkusResultPtr summary) {
  bool purchased = ParseCredentialSummary(summary->message);

  if (purchased && local_state_) {
    local_state_->SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  }

  std::move(callback).Run(purchased, purchased ? "" : "Purchase not found");
}
