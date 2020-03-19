/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/promotion/promotion.h"

#include <map>
#include <memory>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "bat/ledger/internal/bat_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state_keys.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/properties/wallet_info_properties.h"
#include "bat/ledger/internal/request/promotion_requests.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/common/bind_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/promotion/promotion_util.h"
#include "bat/ledger/internal/promotion/promotion_transfer.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

#include "wrapper.hpp"  // NOLINT

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::PublicKey;
using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::UnblindedToken;

namespace braveledger_promotion {

namespace {

void HandleExpiredPromotions(
    bat_ledger::LedgerImpl* ledger_impl,
    ledger::PromotionMap* promotions) {
  DCHECK(promotions);
  if (!promotions) {
    return;
  }

  const uint64_t current_time =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  bool check = false;
  for (auto& item : *promotions) {
    if (!item.second || item.second->status == ledger::PromotionStatus::OVER) {
      continue;
    }

    // we shouldn't expire ad grant
    if (item.second->type == ledger::PromotionType::ADS) {
      continue;
    }

    if (item.second->expires_at > 0 &&
        item.second->expires_at <= current_time)  {
      check = true;
      ledger_impl->UpdatePromotionStatus(
          item.second->id,
          ledger::PromotionStatus::OVER,
          [](const ledger::Result _){});
    }
  }

  if (check) {
    ledger_impl->CheckUnblindedTokensExpiration([](const ledger::Result _){});
  }
}

}  // namespace

Promotion::Promotion(bat_ledger::LedgerImpl* ledger) :
    attestation_(std::make_unique<braveledger_attestation::AttestationImpl>
        (ledger)),
    transfer_(std::make_unique<PromotionTransfer>(ledger)),
    ledger_(ledger) {
  DCHECK(ledger_);
  credentials_ = braveledger_credentials::CredentialsFactory::Create(
      ledger_,
      ledger::CredsBatchType::PROMOTION);
  DCHECK(credentials_);
}

Promotion::~Promotion() = default;

void Promotion::Initialize() {
  if (!ledger_->GetBooleanState(ledger::kStatePromotionCorruptedMigrated)) {
    auto check_callback = std::bind(&Promotion::CheckForCorrupted,
        this,
        _1);

    ledger_->GetAllCredsBatches(check_callback);
  }

  auto retry_callback = std::bind(&Promotion::Retry,
      this,
      _1);

  ledger_->GetAllPromotions(retry_callback);
}

void Promotion::Fetch(ledger::FetchPromotionCallback callback) {
  // make sure wallet/client state is sane here as this is the first
  // panel call.
  const std::string& wallet_payment_id = ledger_->GetPaymentId();
  const std::string& passphrase = ledger_->GetWalletPassphrase();
  if (wallet_payment_id.empty() || passphrase.empty()) {
    ledger::PromotionList empty_list;
    callback(ledger::Result::CORRUPTED_WALLET, std::move(empty_list));
    ledger::WalletProperties properties;
    ledger_->OnWalletProperties(ledger::Result::CORRUPTED_WALLET, properties);
    return;
  }

  auto url_callback = std::bind(&Promotion::OnFetch,
      this,
      _1,
      _2,
      _3,
      std::move(callback));

  auto client_info = ledger_->GetClientInfo();
  const std::string client = ParseClientInfoToString(std::move(client_info));

  const std::string url = braveledger_request_util::GetFetchPromotionUrl(
      wallet_payment_id,
      client);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void Promotion::OnFetch(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    ledger::FetchPromotionCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  ledger::PromotionList list;

  if (response_status_code == net::HTTP_NOT_FOUND) {
    ProcessFetchedPromotions(
        ledger::Result::NOT_FOUND,
        std::move(list),
        callback);
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    ProcessFetchedPromotions(
        ledger::Result::LEDGER_ERROR,
        std::move(list),
        callback);
    return;
  }

  auto all_callback = std::bind(&Promotion::OnGetAllPromotions,
      this,
      _1,
      response,
      callback);

  ledger_->GetAllPromotions(all_callback);
}

void Promotion::OnGetAllPromotions(
    ledger::PromotionMap promotions,
    const std::string& response,
    ledger::FetchPromotionCallback callback) {
  HandleExpiredPromotions(ledger_, &promotions);

  ledger::PromotionList list;
  bool success = ParseFetchResponse(response, &list);

  if (!success) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Failed to parse promotions";
    ProcessFetchedPromotions(
        ledger::Result::LEDGER_ERROR,
        std::move(list),
        callback);
    return;
  }

  for (auto & item : list) {
    auto it = promotions.find(item->id);
    if (it != promotions.end() &&
        it->second->status != ledger::PromotionStatus::ACTIVE) {
      continue;
    }

    // if the server return expiration for ads we need to set it to 0
    if (item->type == ledger::PromotionType::ADS) {
      item->expires_at = 0;
    }

    if (item->legacy_claimed) {
      item->status = ledger::PromotionStatus::ATTESTED;
      auto legacy_callback = std::bind(&Promotion::LegacyClaimedSaved,
          this,
          _1,
          braveledger_bind_util::FromPromotionToString(item->Clone()));
      ledger_->SavePromotion(item->Clone(), legacy_callback);
      continue;
    }

    promotions.insert(std::make_pair(item->id, item->Clone()));

    ledger_->SavePromotion(
        item->Clone(),
        [](const ledger::Result _){});
  }

  ledger::PromotionList promotions_ui;
  for (auto & item : promotions) {
    if (item.second->status == ledger::PromotionStatus::ACTIVE ||
        item.second->status == ledger::PromotionStatus::FINISHED) {
      promotions_ui.push_back(item.second->Clone());
    }
  }

  ProcessFetchedPromotions(
      ledger::Result::LEDGER_OK,
      std::move(promotions_ui),
      callback);
}

void Promotion::LegacyClaimedSaved(
    const ledger::Result result,
    const std::string& promotion_string) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Save failed";
    return;
  }

  auto promotion_ptr =
      braveledger_bind_util::FromStringToPromotion(promotion_string);

  GetCredentials(std::move(promotion_ptr), [](const ledger::Result _){});
}

void Promotion::Claim(
    const std::string& payload,
    ledger::ClaimPromotionCallback callback) {
  attestation_->Start(payload, callback);
}

void Promotion::Attest(
    const std::string& promotion_id,
    const std::string& solution,
    ledger::AttestPromotionCallback callback) {
  auto confirm_callback = std::bind(&Promotion::OnAttestPromotion,
      this,
      _1,
      promotion_id,
      callback);
  attestation_->Confirm(solution, confirm_callback);
}

void Promotion::OnAttestPromotion(
    const ledger::Result result,
    const std::string& promotion_id,
    ledger::AttestPromotionCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
    << "Attestation failed " << result;
    callback(result, nullptr);
    return;
  }

  auto promotion_callback = std::bind(&Promotion::OnCompletedAttestation,
      this,
      _1,
      callback);

  ledger_->GetPromotion(promotion_id, promotion_callback);
}

void Promotion::OnCompletedAttestation(
    ledger::PromotionPtr promotion,
    ledger::AttestPromotionCallback callback) {
  if (!promotion) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
    << "Promotion does not exist ";
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  if (promotion->status == ledger::PromotionStatus::FINISHED) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Promotions already claimed";
    callback(ledger::Result::GRANT_ALREADY_CLAIMED, nullptr);
    return;
  }

  promotion->status = ledger::PromotionStatus::ATTESTED;

  auto save_callback = std::bind(&Promotion::AttestedSaved,
      this,
      _1,
      braveledger_bind_util::FromPromotionToString(promotion->Clone()),
      callback);

  ledger_->SavePromotion(promotion->Clone(), save_callback);
}

void Promotion::AttestedSaved(
    const ledger::Result result,
    const std::string& promotion_string,
    ledger::AttestPromotionCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Save failed ";
    callback(result, nullptr);
    return;
  }

  auto promotion_ptr =
      braveledger_bind_util::FromStringToPromotion(promotion_string);

  if (!promotion_ptr) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  auto claim_callback = std::bind(&Promotion::Complete,
      this,
      _1,
      promotion_ptr->id,
      callback);

  GetCredentials(std::move(promotion_ptr), claim_callback);
}

void Promotion::Complete(
    const ledger::Result result,
    const std::string& promotion_id,
    ledger::AttestPromotionCallback callback) {
  auto promotion_callback = std::bind(&Promotion::OnComplete,
      this,
      _1,
      result,
      callback);
  ledger_->GetPromotion(promotion_id, promotion_callback);
}

void Promotion::OnComplete(
    ledger::PromotionPtr promotion,
    const ledger::Result result,
    ledger::AttestPromotionCallback callback) {
  if (promotion && result == ledger::Result::LEDGER_OK) {
    ledger_->SetBalanceReportItem(
        braveledger_time_util::GetCurrentMonth(),
        braveledger_time_util::GetCurrentYear(),
        ConvertPromotionTypeToReportType(promotion->type),
        promotion->approximate_value);
  }

  callback(result, std::move(promotion));
}

void Promotion::ProcessFetchedPromotions(
    const ledger::Result result,
    ledger::PromotionList promotions,
    ledger::FetchPromotionCallback callback) {
  const uint64_t now = static_cast<uint64_t>(base::Time::Now().ToDoubleT());
  ledger_->SetUint64State(ledger::kStatePromotionLastFetchStamp, now);
  last_check_timer_id_ = 0;
  const bool retry = result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND;
  Refresh(retry);
  callback(result, std::move(promotions));
}

void Promotion::OnTimer(const uint32_t timer_id) {
  if (timer_id == last_check_timer_id_) {
    last_check_timer_id_ = 0;
    Fetch([](ledger::Result _, ledger::PromotionList __){});
    return;
  }

  if (timer_id == retry_timer_id_) {
    auto claim_callback = std::bind(&Promotion::Retry,
      this,
      _1);
    ledger_->GetAllPromotions(claim_callback);
  }
}

void Promotion::GetCredentials(
    ledger::PromotionPtr promotion,
    ledger::ResultCallback callback) {
  if (!promotion) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Promotion is null";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  braveledger_credentials::CredentialsTrigger trigger;
  trigger.id = promotion->id;
  trigger.size = promotion->suggestions;
  trigger.type = ledger::CredsBatchType::PROMOTION;

  auto creds_callback = std::bind(&Promotion::CredentialsProcessed,
      this,
      _1,
      promotion->id,
      callback);

  credentials_->Start(trigger, creds_callback);
}

void Promotion::CredentialsProcessed(
    const ledger::Result result,
    const std::string& promotion_id,
    ledger::ResultCallback callback) {
  if (result == ledger::Result::RETRY) {
    ledger_->SetTimer(5, &retry_timer_id_);
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  if (result != ledger::Result::LEDGER_OK) {
    callback(result);
    return;
  }

  ledger_->UpdatePromotionStatus(
      promotion_id,
      ledger::PromotionStatus::FINISHED,
      callback);
}

void Promotion::Retry(ledger::PromotionMap promotions) {
  HandleExpiredPromotions(ledger_, &promotions);

  for (auto& promotion : promotions) {
    if (!promotion.second) {
      continue;
    }

    switch (promotion.second->status) {
      case ledger::PromotionStatus::ATTESTED: {
        GetCredentials(
            std::move(promotion.second),
            [](const ledger::Result _){});
        break;
      }
      case ledger::PromotionStatus::ACTIVE:
      case ledger::PromotionStatus::FINISHED:
      case ledger::PromotionStatus::OVER: {
        break;
      }
    }
  }
}

void Promotion::Refresh(const bool retry_after_error) {
  uint64_t start_timer_in = 0ull;
  if (last_check_timer_id_ != 0) {
    return;
  }

  if (retry_after_error) {
    start_timer_in = brave_base::random::Geometric(300);

    BLOG(ledger_, ledger::LogLevel::LOG_WARNING) <<
      "Failed to refresh promotion, will try again in " << start_timer_in;
  } else {
    const auto default_time = braveledger_ledger::_promotion_load_interval;
    const uint64_t now = static_cast<uint64_t>(base::Time::Now().ToDoubleT());
    const uint64_t last_promo_stamp =
        ledger_->GetUint64State(ledger::kStatePromotionLastFetchStamp);

    uint64_t time_since_last_promo_check = 0ull;

    if (last_promo_stamp != 0ull && last_promo_stamp < now) {
      time_since_last_promo_check = now - last_promo_stamp;
    }

    if (now == last_promo_stamp) {
      start_timer_in = default_time;
    } else if (time_since_last_promo_check > 0 &&
        default_time > time_since_last_promo_check) {
      start_timer_in = default_time - time_since_last_promo_check;
    }
  }

  ledger_->SetTimer(start_timer_in, &last_check_timer_id_);
}

void Promotion::CheckForCorrupted(ledger::CredsBatchList list) {
  if (list.empty()) {
    return;
  }

  std::vector<std::string> corrupted_promotions;

  for (auto& item : list) {
    if (!item ||
        (item->status != ledger::CredsBatchStatus::SIGNED &&
         item->status != ledger::CredsBatchStatus::FINISHED)) {
      continue;
    }

    std::vector<std::string> unblinded_encoded_tokens;
    std::string error;
    bool result = braveledger_credentials::UnBlindCreds(
        *item,
        &unblinded_encoded_tokens,
        &error);

    if (!result) {
      BLOG(ledger_, ledger::LogLevel::LOG_INFO)
          << "Promotion corrupted " << item->trigger_id;
      corrupted_promotions.push_back(item->trigger_id);
    }
  }

  if (corrupted_promotions.empty()) {
    ledger_->SetBooleanState(ledger::kStatePromotionCorruptedMigrated, true);
    return;
  }

  auto get_callback = std::bind(&Promotion::CorruptedPromotions,
      this,
      _1,
      corrupted_promotions);

  ledger_->GetPromotionList(corrupted_promotions, get_callback);
}

void Promotion::CorruptedPromotions(
    ledger::PromotionList promotions,
    const std::vector<std::string>& ids) {
  base::Value corrupted_claims(base::Value::Type::LIST);

  for (auto& item : promotions) {
    if (!item) {
      continue;
    }

    corrupted_claims.Append(base::Value(item->claim_id));
  }

  if (corrupted_claims.GetList().empty()) {
    return;
  }

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetKey("claimIds", std::move(corrupted_claims));

  std::string json;
  base::JSONWriter::Write(body, &json);

  const std::string url = braveledger_request_util::ReportClobberedClaimsUrl();

  auto url_callback = std::bind(&Promotion::OnCheckForCorrupted,
      this,
      _1,
      _2,
      _3,
      ids);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void Promotion::OnCheckForCorrupted(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::vector<std::string>& promotion_id_list) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    return;
  }

  auto save_callback = std::bind(&Promotion::PromotionListDeleted,
      this,
      _1);

  ledger_->DeletePromotionList(promotion_id_list, save_callback);
}

void Promotion::PromotionListDeleted(const ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Error deleting promotions";
    return;
  }

  ledger_->SetBooleanState(ledger::kStatePromotionCorruptedMigrated, true);
}

void Promotion::TransferTokens(
    ledger::ExternalWalletPtr wallet,
    ledger::ResultCallback callback) {
  transfer_->Start(std::move(wallet), callback);
}

}  // namespace braveledger_promotion
