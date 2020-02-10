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
#include "bat/ledger/internal/common/security_helper.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/promotion/promotion_util.h"
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
    bat_ledger::LedgerImpl* ledger,
    ledger::PromotionMap* promotions) {
  DCHECK(promotions);
  if (!promotions) {
    return;
  }

  const uint64_t current_time =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  for (auto& item : *promotions) {
    if (!item.second) {
      continue;
    }

    // we shouldn't expire ad grant
    if (item.second->type == ledger::PromotionType::ADS) {
      continue;
    }

    if (item.second->expires_at > 0 &&
        item.second->expires_at <= current_time)  {
      item.second->status = ledger::PromotionStatus::OVER;

      ledger->DeleteUnblindedTokensForPromotion(item.second->id,
          [](const ledger::Result _){});
    }
  }
}

}  // namespace

Promotion::Promotion(bat_ledger::LedgerImpl* ledger) :
    attestation_(std::make_unique<braveledger_attestation::AttestationImpl>
        (ledger)),
    ledger_(ledger) {
}

Promotion::~Promotion() = default;

void Promotion::Initialize() {
  if (!ledger_->GetBooleanState(ledger::kStatePromotionCorruptedMigrated)) {
    auto check_callback = std::bind(&Promotion::CheckForCorrupted,
        this,
        _1);

    ledger_->GetAllPromotions(check_callback);
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

  ClaimTokens(std::move(promotion_ptr), [](const ledger::Result _){});
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

  if (promotion->status == ledger::PromotionStatus::CLAIMED) {
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

  ClaimTokens(std::move(promotion_ptr), claim_callback);
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

void Promotion::Retry(ledger::PromotionMap promotions) {
  HandleExpiredPromotions(ledger_, &promotions);

  for (auto& promotion : promotions) {
    if (!promotion.second) {
      continue;
    }

    switch (promotion.second->status) {
      case ledger::PromotionStatus::ATTESTED: {
        ClaimTokens(promotion.second->Clone(), [](const ledger::Result _){});
        break;
      }
      case ledger::PromotionStatus::CLAIMED: {
        FetchSignedTokens(
          promotion.second->Clone(),
          [](const ledger::Result _){});
        break;
      }
      case ledger::PromotionStatus::SIGNED_TOKENS: {
        OnProcessSignedCredentials(
          promotion.second->Clone(),
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

void Promotion::ClaimTokens(
    ledger::PromotionPtr promotion,
    ledger::ResultCallback callback) {
  if (!promotion) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (!promotion->credentials) {
    promotion->credentials = ledger::PromotionCreds::New();
  }

  const auto tokens =
      braveledger_helper::Security::GenerateTokens(promotion->suggestions);

  base::Value tokens_list(base::Value::Type::LIST);
  for (auto & token : tokens) {
    auto token_base64 = token.encode_base64();
    auto token_value = base::Value(token_base64);
    tokens_list.GetList().push_back(std::move(token_value));
  }
  std::string json_tokens;
  base::JSONWriter::Write(tokens_list, &json_tokens);

  const auto blinded_tokens =
      braveledger_helper::Security::BlindTokens(tokens);

  if (blinded_tokens.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Blinded tokens are empty";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string payment_id = ledger_->GetPaymentId();

  base::Value blinded_list(base::Value::Type::LIST);
  for (auto & token : blinded_tokens) {
    auto token_base64 = token.encode_base64();
    auto token_value = base::Value(token_base64);
    blinded_list.GetList().push_back(std::move(token_value));
  }
  std::string json_blinded;
  base::JSONWriter::Write(blinded_list, &json_blinded);

  promotion->credentials->tokens = json_tokens;
  promotion->credentials->blinded_creds = json_blinded;

  auto save_callback = std::bind(&Promotion::ClaimTokensSaved,
      this,
      _1,
      braveledger_bind_util::FromPromotionToString(promotion->Clone()),
      callback);

  ledger_->SavePromotion(promotion->Clone(), save_callback);
}

void Promotion::ClaimTokensSaved(
    const ledger::Result result,
    const std::string& promotion_string,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Save failed";
    callback(result);
    return;
  }

  auto promotion =
      braveledger_bind_util::FromStringToPromotion(promotion_string);

  if (!promotion) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto blinded_tokens = ParseStringToBaseList(
      promotion->credentials->blinded_creds);

  if (!blinded_tokens) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
    << "Blinded tokens are corrupted";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("paymentId", ledger_->GetPaymentId());
  body.SetKey("blindedCreds", base::Value(std::move(*blinded_tokens)));

  std::string json;
  base::JSONWriter::Write(body, &json);

  ledger::WalletInfoProperties wallet_info = ledger_->GetWalletInfo();

  const auto headers = braveledger_request_util::BuildSignHeaders(
      "post /v1/promotions/" + promotion->id,
      json,
      ledger_->GetPaymentId(),
      wallet_info.key_info_seed);

  const std::string url =
      braveledger_request_util::ClaimTokensUrl(promotion->id);
  auto url_callback = std::bind(&Promotion::OnClaimTokens,
      this,
      _1,
      _2,
      _3,
      braveledger_bind_util::FromPromotionToString(std::move(promotion)),
      callback);

  ledger_->LoadURL(
      url,
      headers,
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void Promotion::OnClaimTokens(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::string& promotion_string,
    ledger::ResultCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  auto promotion =
      braveledger_bind_util::FromStringToPromotion(promotion_string);

  if (response_status_code != net::HTTP_OK || !promotion) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const auto claim_id = ParseClaimTokenResponse(response);

  if (claim_id.empty() || !promotion->credentials) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Corrupted data";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  promotion->status = ledger::PromotionStatus::CLAIMED;
  promotion->credentials->claim_id = claim_id;

  auto save_callback = std::bind(&Promotion::ClaimedTokensSaved,
      this,
      _1,
      braveledger_bind_util::FromPromotionToString(promotion->Clone()),
      callback);

  ledger_->SavePromotion(promotion->Clone(), save_callback);
}

void Promotion::ClaimedTokensSaved(
    const ledger::Result result,
    const std::string& promotion_string,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    ledger_->SetTimer(brave_base::random::Geometric(60), &retry_timer_id_);
    return;
  }

  auto promotion =
      braveledger_bind_util::FromStringToPromotion(promotion_string);

  FetchSignedTokens(std::move(promotion), callback);
}

void Promotion::FetchSignedTokens(
    ledger::PromotionPtr promotion,
    ledger::ResultCallback callback) {
  if (!promotion || !promotion->credentials) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Corrupted data";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string url = braveledger_request_util::FetchSignedTokensUrl(
      promotion->id,
      promotion->credentials->claim_id);
  auto url_callback = std::bind(&Promotion::OnFetchSignedTokens,
      this,
      _1,
      _2,
      _3,
      braveledger_bind_util::FromPromotionToString(std::move(promotion)),
      callback);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void Promotion::OnFetchSignedTokens(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::string& promotion_string,
    ledger::ResultCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  auto promotion =
      braveledger_bind_util::FromStringToPromotion(promotion_string);

  if (response_status_code == net::HTTP_ACCEPTED) {
    callback(ledger::Result::LEDGER_OK);
    ledger_->SetTimer(5, &retry_timer_id_);
    return;
  }

  if (!promotion ||
      !promotion->credentials ||
      response_status_code != net::HTTP_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Corrupted data";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  base::Value parsed_response(base::Value::Type::DICTIONARY);
  ParseSignedTokensResponse(response, &parsed_response);

  if (parsed_response.DictSize() != 3) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Parsing failed";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  std::string json_creds;
  base::JSONWriter::Write(
      *parsed_response.FindListKey("signed_creds"),
      &json_creds);
  promotion->credentials->signed_creds = json_creds;
  promotion->credentials->public_key =
      *parsed_response.FindStringKey("public_key");
  promotion->credentials->batch_proof =
      *parsed_response.FindStringKey("batch_proof");

  promotion->status = ledger::PromotionStatus::SIGNED_TOKENS;

  auto process_callback =
        std::bind(&Promotion::ProcessSignedCredentials,
            this,
            _1,
            promotion->id,
            callback);

  ledger_->SavePromotion(
        promotion->Clone(),
        process_callback);
}

void Promotion::ProcessSignedCredentials(
    const ledger::Result result,
    const std::string& promotion_id,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Save failed";
    callback(result);
    return;
  }

  auto promotion_callback = std::bind(&Promotion::OnProcessSignedCredentials,
      this,
      _1,
      callback);

  ledger_->GetPromotion(promotion_id, promotion_callback);
}

void Promotion::OnProcessSignedCredentials(
    ledger::PromotionPtr promotion,
    ledger::ResultCallback callback) {
  if (!promotion) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bool result = VerifyPublicKey(promotion->Clone());

  if (!result) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
    << "Public key verification failed";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  std::vector<std::string> unblinded_encoded_tokens;
  std::string error;
  if (ledger::is_testing) {
    result = UnBlindTokensMock(promotion->Clone(), &unblinded_encoded_tokens);
  } else {
    result = UnBlindTokens(
        promotion->Clone(),
        &unblinded_encoded_tokens,
        &error);
  }

  const auto signed_creds = ParseStringToBaseList(
      promotion->credentials->signed_creds);

  if (!result) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "UnBlindTokens: " << error;
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  SaveUnblindedTokens(std::move(promotion), unblinded_encoded_tokens, callback);
}

void Promotion::SaveUnblindedTokens(
    ledger::PromotionPtr promotion,
    const std::vector<std::string>& unblinded_encoded_tokens,
    ledger::ResultCallback callback) {
  if (!promotion) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const double value = promotion->approximate_value / promotion->suggestions;
  ledger::UnblindedTokenList list;
  for (auto & token : unblinded_encoded_tokens) {
    auto token_info = ledger::UnblindedToken::New();
    token_info->token_value = token;
    token_info->public_key = promotion->credentials->public_key;
    token_info->value = value;
    token_info->promotion_id = promotion->id;
    list.push_back(std::move(token_info));
  }

  auto save_callback = std::bind(&Promotion::FinishPromotion,
      this,
      _1,
      braveledger_bind_util::FromPromotionToString(std::move(promotion)),
      callback);

  ledger_->SaveUnblindedTokenList(std::move(list), save_callback);
}

void Promotion::FinishPromotion(
    const ledger::Result result,
    const std::string& promotion_string,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Save failed";
    callback(result);
    return;
  }

  auto promotion =
      braveledger_bind_util::FromStringToPromotion(promotion_string);

  if (!promotion) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const uint64_t current_time =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());
  promotion->status = ledger::PromotionStatus::FINISHED;
  promotion->claimed_at = current_time;
  ledger_->SavePromotion(
        std::move(promotion),
        [](const ledger::Result _){});
  callback(ledger::Result::LEDGER_OK);
  ledger_->UnblindedTokensReady();
}

void Promotion::CheckForCorrupted(const ledger::PromotionMap& promotions) {
  base::Value corrupted_claims(base::Value::Type::LIST);
  std::vector<std::string> corrupted_promotions;

  for (auto & item : promotions) {
    if (!item.second || !item.second->credentials) {
      continue;
    }

    const auto signed_creds = ParseStringToBaseList(
        item.second->credentials->signed_creds);

    std::vector<std::string> unblinded_encoded_tokens;
    std::string error;
    bool result =
        UnBlindTokens(item.second->Clone(), &unblinded_encoded_tokens, &error);
    if (!result) {
      BLOG(ledger_, ledger::LogLevel::LOG_INFO)
      << "Promotion corrupted " << item.second->id;
      corrupted_claims.GetList().push_back(
          base::Value(item.second->credentials->claim_id));
      corrupted_promotions.push_back(item.second->id);
    }
  }

  if (corrupted_claims.GetList().empty()) {
    ledger_->SetBooleanState(ledger::kStatePromotionCorruptedMigrated, true);
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
      corrupted_promotions);

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

}  // namespace braveledger_promotion
