/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/promotion/promotion.h"

#include <map>
#include <memory>
#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ledger/internal/bat_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/state_keys.h"
#include "bat/ledger/internal/static_values.h"
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

Promotion::Promotion(bat_ledger::LedgerImpl* ledger) :
    attestation_(std::make_unique<braveledger_attestation::AttestationImpl>
        (ledger)),
    ledger_(ledger) {
}

Promotion::~Promotion() = default;

void Promotion::Initialize() {
  auto claim_callback = std::bind(&Promotion::Retry,
      this,
      _1);
  ledger_->GetAllPromotions(claim_callback);
}

void Promotion::Fetch(ledger::FetchPromotionCallback callback) {
  // make sure wallet/client state is sane here as this is the first
  // panel call.
  const std::string& wallet_payment_id = ledger_->GetPaymentId();
  const std::string& passphrase = ledger_->GetWalletPassphrase();
  if (wallet_payment_id.empty() || passphrase.empty()) {
    ledger::PromotionList empty_list;
    callback(ledger::Result::CORRUPTED_WALLET, std::move(empty_list));
    braveledger_bat_helper::WALLET_PROPERTIES_ST properties;
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

  if (list.size() > 0) {
    for (auto & item : list) {
      auto it = promotions.find(item->id);
      if (it != promotions.end()) {
        if (promotions.at(item->id)->status !=
            ledger::PromotionStatus::ACTIVE) {
          item->status = promotions.at(item->id)->status;
        }
      } else {
        promotions.insert(std::make_pair(item->id, item->Clone()));
      }

      ledger_->InsertOrUpdatePromotion(
          item->Clone(),
          [](const ledger::Result _){});
    }
  }


  ledger::PromotionList promotions_ui;
  for (auto & item : promotions) {
    if (item.second->status == ledger::PromotionStatus::ACTIVE ||
        item.second->status == ledger::PromotionStatus::ATTESTED ||
        item.second->status == ledger::PromotionStatus::FINISHED) {
      promotions_ui.push_back(item.second->Clone());
    }
  }

  ProcessFetchedPromotions(
      ledger::Result::LEDGER_OK,
      std::move(promotions_ui),
      callback);
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
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  if (promotion->status == ledger::PromotionStatus::CLAIMED) {
    callback(ledger::Result::GRANT_ALREADY_CLAIMED, nullptr);
    return;
  }

  promotion->status = ledger::PromotionStatus::ATTESTED;
  ledger_->InsertOrUpdatePromotion(
        promotion->Clone(),
        [](const ledger::Result _){});

  auto claim_callback = std::bind(&Promotion::Complete,
      this,
      _1,
      braveledger_bind_util::FromPromotionToString(promotion->Clone()),
      callback);

  ClaimTokens(std::move(promotion), claim_callback);
}

void Promotion::Complete(
    const ledger::Result result,
    const std::string& promotion_string,
    ledger::AttestPromotionCallback callback) {
  auto promotion_ptr =
      braveledger_bind_util::FromStringToPromotion(promotion_string);
  const std::string probi = braveledger_bat_util::ConvertToProbi(
      std::to_string(promotion_ptr->approximate_value));
  if (result == ledger::Result::LEDGER_OK) {
    ledger_->SetBalanceReportItem(
      braveledger_time_util::GetCurrentMonth(),
      braveledger_time_util::GetCurrentYear(),
      ConvertPromotionTypeToReportType(promotion_ptr->type),
      probi);
  }

  callback(result, std::move(promotion_ptr));
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
  for (auto & promotion : promotions) {
    if (promotion.second->status == ledger::PromotionStatus::CLAIMED) {
      FetchSignedTokens(
          promotion.second->Clone(),
          [](const ledger::Result _){});
      continue;
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

  if (blinded_tokens.size() == 0) {
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
  ledger_->InsertOrUpdatePromotion(
        promotion->Clone(),
        [](const ledger::Result _){});

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("paymentId", ledger_->GetPaymentId());
  body.SetKey("blindedCreds", std::move(blinded_list));

  std::string json;
  base::JSONWriter::Write(body, &json);

  braveledger_bat_helper::WALLET_INFO_ST wallet_info = ledger_->GetWalletInfo();

  const auto headers = braveledger_request_util::BuildSignHeaders(
      "post /v1/promotions/" + promotion->id,
      json,
      ledger_->GetPaymentId(),
      wallet_info.keyInfoSeed_);

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
    const std::string promotion_string,
    ledger::ResultCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  auto promotion =
      braveledger_bind_util::FromStringToPromotion(promotion_string);

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const auto claim_id = ParseClaimTokenResponse(response);

  if (claim_id.empty() || !promotion->credentials) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  promotion->status = ledger::PromotionStatus::CLAIMED;
  promotion->credentials->claim_id = claim_id;
  ledger_->InsertOrUpdatePromotion(
        promotion->Clone(),
        [](const ledger::Result _){});

  FetchSignedTokens(std::move(promotion), callback);
}

void Promotion::FetchSignedTokens(
    ledger::PromotionPtr promotion,
    ledger::ResultCallback callback) {
  if (!promotion || !promotion->credentials) {
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
    const std::string promotion_string,
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
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  base::Value parsed_response(base::Value::Type::DICTIONARY);
  ParseSignedTokensResponse(response, &parsed_response);

  if (parsed_response.DictSize() != 3) {
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
  ledger_->InsertOrUpdatePromotion(
        promotion->Clone(),
        [](const ledger::Result _){});

  bool result = VerifyPublicKey(promotion->Clone());

  if (!result) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  std::vector<std::string> unblinded_encoded_tokens;
  if (ledger::is_testing) {
    result = UnBlindTokensMock(promotion->Clone(), &unblinded_encoded_tokens);
  } else {
    result = UnBlindTokens(promotion->Clone(), &unblinded_encoded_tokens);
  }

  if (!result) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  FinishPromotion(
      std::move(promotion),
      unblinded_encoded_tokens,
      callback);
}

bool Promotion::UnBlindTokens(
    ledger::PromotionPtr promotion,
    std::vector<std::string>* unblinded_encoded_tokens) {
  if (!promotion || !promotion->credentials || !unblinded_encoded_tokens) {
    return false;
  }

  auto batch_proof =
      BatchDLEQProof::decode_base64(promotion->credentials->batch_proof);

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "UnBlindTokens: " << e.what();
    return false;
  }

  auto tokens_base64 = ParseStringToBaseList(promotion->credentials->tokens);
  std::vector<Token> tokens;
  for (auto& item : *tokens_base64) {
    const auto token = Token::decode_base64(item.GetString());
    tokens.push_back(token);
  }

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "UnBlindTokens: " << e.what();
    return false;
  }

  auto blinded_tokens_base64 = ParseStringToBaseList(
      promotion->credentials->blinded_creds);
  std::vector<BlindedToken> blinded_tokens;
  for (auto& item : *blinded_tokens_base64) {
    const auto blinded_token = BlindedToken::decode_base64(item.GetString());
    blinded_tokens.push_back(blinded_token);
  }

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "UnBlindTokens: " << e.what();
    return false;
  }

  auto signed_tokens_base64 = ParseStringToBaseList(
      promotion->credentials->signed_creds);
  std::vector<SignedToken> signed_tokens;
  for (auto& item : *signed_tokens_base64) {
    const auto signed_token = SignedToken::decode_base64(item.GetString());
    signed_tokens.push_back(signed_token);
  }

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "UnBlindTokens: " << e.what();
    return false;
  }

  const auto public_key = PublicKey::decode_base64(
      promotion->credentials->public_key);

  auto unblinded_tokens = batch_proof.verify_and_unblind(
     tokens,
     blinded_tokens,
     signed_tokens,
     public_key);

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "UnBlindTokens: " << e.what();
    return false;
  }

  for (auto & token : unblinded_tokens) {
    unblinded_encoded_tokens->push_back(token.encode_base64());
  }

  return true;
}

void Promotion::FinishPromotion(
    ledger::PromotionPtr promotion,
    const std::vector<std::string>& unblinded_encoded_tokens,
    ledger::ResultCallback callback) {
  const double value = promotion->approximate_value / promotion->suggestions;
  for (auto & token : unblinded_encoded_tokens) {
    auto token_info = ledger::UnblindedToken::New();
    token_info->token_value = token;
    token_info->public_key = promotion->credentials->public_key;
    token_info->value = value;
    token_info->promotion_id = promotion->id;
    ledger_->InsertOrUpdateUnblindedToken(
        std::move(token_info),
        [](const ledger::Result _){});
  }

  promotion->status = ledger::PromotionStatus::FINISHED;
  ledger_->InsertOrUpdatePromotion(
        std::move(promotion),
        [](const ledger::Result _){});
  callback(ledger::Result::LEDGER_OK);
  ledger_->UnblindedTokensReady();
}

}  // namespace braveledger_promotion
