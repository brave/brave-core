/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/promotion.h"

#include <map>
#include <memory>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state_keys.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/request/promotion_requests.h"
#include "bat/ledger/internal/request/request_util.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_promotion {

Promotion::Promotion(bat_ledger::LedgerImpl* ledger) :
    attestation_(std::make_unique<braveledger_attestation::AttestationImpl>
        (ledger)),
    ledger_(ledger) {
}

Promotion::~Promotion() {
}

ledger::PromotionType ConvertStringToPromotionType(const std::string& type) {
  if (type == "ugp") {
    return ledger::PromotionType::UGP;
  }

  if (type == "ads") {
    return ledger::PromotionType::ADS;
  }

  // unknown promotion type, returning dummy value.
  NOTREACHED();
  return ledger::PromotionType::UGP;
}

bool ParseFetchResponse(
    const std::string& response,
    ledger::PromotionList* list) {
  if (!list) {
    return false;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  auto* promotions = dictionary->FindKey("promotions");
  if (promotions && promotions->is_list()) {
    for (auto& item : promotions->GetList()) {
      ledger::PromotionPtr promotion = ledger::Promotion::New();

      auto* id = item.FindKey("id");
      if (!id || !id->is_string()) {
        continue;
      }
      promotion->id = id->GetString();

      auto* version = item.FindKey("version");
      if (!version || !version->is_int()) {
        continue;
      }
      promotion->version = version->GetInt();

      auto* type = item.FindKey("type");
      if (!type || !type->is_string()) {
        continue;
      }
      promotion->type = ConvertStringToPromotionType(type->GetString());

      auto* suggestions = item.FindKey("suggestionsPerGrant");
      if (!suggestions || !suggestions->is_int()) {
        continue;
      }
      promotion->suggestions = suggestions->GetInt();

      auto* approximate_value = item.FindKey("approximateValue");
      if (!approximate_value || !approximate_value->is_string()) {
        continue;
      }
      promotion->approximate_value =std::stod(approximate_value->GetString());

      auto* expires_at = item.FindKey("expiresAt");
      if (!expires_at || !expires_at->is_string()) {
        continue;
      }

      base::Time time;
      bool success =
          base::Time::FromUTCString(expires_at->GetString().c_str(), &time);
      if (success) {
        promotion->expires_at = time.ToDoubleT();
      }

      list->push_back(std::move(promotion));
    }
  }

  return true;
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

  const std::string url = braveledger_request_util::GetFetchPromotionUrl(
      wallet_payment_id,
      "osx"); // TODO make dynamic

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
    ledger_->InsertOrUpdatePromotion(
        item->Clone(),
        [](const ledger::Result _){});
  }

  ProcessFetchedPromotions(
      ledger::Result::LEDGER_OK,
      std::move(list),
      callback);
}

void Promotion::ClaimPromotion(
    const std::string& payload,
    ledger::ClaimPromotionCallback callback) {
  attestation_->Start(payload, callback);
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

void Promotion::SetGrant(const std::string& captchaResponse,
                      const std::string& promotionId,
                      const std::string& safetynet_token) {
  if (promotionId.empty() && safetynet_token.empty()) {
    braveledger_bat_helper::GRANT properties;
    ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, properties);
    return;
  }

  std::string keys[2] = {"promotionId", "captchaResponse"};
  std::string values[2] = {promotionId, captchaResponse};
  std::string payload = braveledger_bat_helper::stringify(keys, values,
      safetynet_token.empty() ? 2 : 1);

  std::vector<std::string> headers;
  if (!safetynet_token.empty()) {
    headers.push_back("safetynet-token:" + safetynet_token);
  }

//  auto callback = std::bind(&Promotion::SetGrantCallback, this, _1, _2, _3,
//                                !safetynet_token.empty());
//  ledger_->LoadURL(braveledger_request_util::BuildUrl(
//        (std::string)GET_SET_PROMOTION + "/" + ledger_->GetPaymentId(),
//        safetynet_token.empty() ? PREFIX_V2 : PREFIX_V3),
//      headers, payload, "application/json; charset=utf-8",
//      ledger::UrlMethod::PUT, callback);
}

void Promotion::SetGrantCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    bool is_safetynet_check) {
  std::string error;
  unsigned int statusCode;
  braveledger_bat_helper::GRANT grant;
  braveledger_bat_helper::getJSONResponse(response, &statusCode, &error);

  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (!error.empty()) {
    if (statusCode == net::HTTP_FORBIDDEN) {
      ledger_->OnGrantFinish(is_safetynet_check ?
          ledger::Result::SAFETYNET_ATTESTATION_FAILED :
          ledger::Result::CAPTCHA_FAILED, grant);
    } else if (statusCode == net::HTTP_NOT_FOUND ||
               statusCode == net::HTTP_GONE) {
      ledger_->OnGrantFinish(ledger::Result::NOT_FOUND, grant);
    } else if (statusCode == net::HTTP_CONFLICT) {
      ledger_->OnGrantFinish(ledger::Result::GRANT_ALREADY_CLAIMED, grant);
    } else {
      ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, grant);
    }
    return;
  }

//  bool ok = braveledger_bat_helper::loadFromJson(&grant, response);
//  if (!ok) {
//    ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, grant);
//    return;
//  }

  braveledger_bat_helper::Grants updated_grants;
  braveledger_bat_helper::Grants state_grants = ledger_->GetGrants();

  for (auto state_grant : state_grants) {
    if (grant.type == state_grant.type) {
      grant.promotionId = state_grant.promotionId;
      ledger_->OnGrantFinish(ledger::Result::LEDGER_OK, grant);
      updated_grants.push_back(grant);
    } else {
      updated_grants.push_back(state_grant);
    }
  }

  ledger_->SetGrants(updated_grants);
}

void Promotion::OnTimer(const uint32_t timer_id) {
  if (timer_id == last_check_timer_id_) {
    last_check_timer_id_ = 0;
    Fetch([](ledger::Result _, ledger::PromotionList __){});
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
      "Failed to refresh grant, will try again in " << start_timer_in;
  } else {
    const uint64_t now = static_cast<uint64_t>(base::Time::Now().ToDoubleT());
    const uint64_t last_grant_stamp =
        ledger_->GetUint64State(ledger::kStatePromotionLastFetchStamp);

    uint64_t time_since_last_grant_check = 0ull;

    if (last_grant_stamp != 0ull && last_grant_stamp < now) {
      time_since_last_grant_check = now - last_grant_stamp;
    }

    if (now == last_grant_stamp) {
      start_timer_in = braveledger_ledger::_grant_load_interval;
    } else if (time_since_last_grant_check > 0 &&
      braveledger_ledger::_grant_load_interval > time_since_last_grant_check) {
      start_timer_in =
        braveledger_ledger::_grant_load_interval - time_since_last_grant_check;
    }
  }

  ledger_->SetTimer(start_timer_in, &last_check_timer_id_);
}

}  // namespace braveledger_promotion
