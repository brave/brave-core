/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_available/get_available.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/promotion/promotion_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace promotion {

GetAvailable::GetAvailable(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetAvailable::~GetAvailable() = default;

std::string GetAvailable::GetUrl(const std::string& platform) {
  const auto wallet = ledger_->wallet()->GetWallet();
  std::string payment_id;
  if (wallet) {
    payment_id = base::StringPrintf(
      "&paymentId=%s",
      wallet->payment_id.c_str());
  }

  const std::string& arguments = base::StringPrintf(
      "migrate=true%s&platform=%s",
      payment_id.c_str(),
      platform.c_str());

  const std::string& path = base::StringPrintf(
      "/v1/promotions?%s",
      arguments.c_str());

  return GetServerUrl(path);
}

type::Result GetAvailable::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid paymentId or platform in request");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized paymentId/promotion combination");
    return type::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

type::Result GetAvailable::ParseBody(
    const std::string& body,
    type::PromotionList* list,
    std::vector<std::string>* corrupted_promotions) {
  DCHECK(list && corrupted_promotions);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  auto* promotions = dictionary->FindListKey("promotions");
  if (!promotions) {
    return type::Result::LEDGER_OK;
  }

  const auto promotion_size = promotions->GetListDeprecated().size();
  for (auto& item : promotions->GetListDeprecated()) {
    type::PromotionPtr promotion = type::Promotion::New();

    const auto* id = item.FindStringKey("id");
    if (!id) {
      continue;
    }
    promotion->id = *id;

    const auto version = item.FindIntKey("version");
    if (!version) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }
    promotion->version = *version;

    const auto* type = item.FindStringKey("type");
    if (!type) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }
    promotion->type =
        ::ledger::promotion::ConvertStringToPromotionType(*type);

    const auto suggestions = item.FindIntKey("suggestionsPerGrant");
    if (!suggestions) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }
    promotion->suggestions = *suggestions;

    const auto* approximate_value = item.FindStringKey("approximateValue");
    if (!approximate_value) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }

    const bool success_conversion =
        base::StringToDouble(*approximate_value, &promotion->approximate_value);
    if (!success_conversion) {
      promotion->approximate_value = 0.0;
    }

    const auto available = item.FindBoolKey("available");
    if (!available) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }

    if (*available) {
      promotion->status = type::PromotionStatus::ACTIVE;
    } else {
      promotion->status = type::PromotionStatus::OVER;
    }

    promotion->created_at = base::Time::Now().ToDoubleT();
    if (auto* created_at = item.FindStringKey("createdAt")) {
      base::Time time;
      if (base::Time::FromUTCString(created_at->c_str(), &time)) {
        promotion->created_at = time.ToDoubleT();
      }
    }

    auto* expires_at = item.FindStringKey("expiresAt");
    if (!expires_at) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }

    base::Time time;
    bool success = base::Time::FromUTCString((*expires_at).c_str(), &time);
    if (success) {
      promotion->expires_at = time.ToDoubleT();
    }

    auto* claimable_until = item.FindStringKey("claimableUntil");
    if (claimable_until) {
      base::Time time;
      if (base::Time::FromUTCString(claimable_until->c_str(), &time)) {
        promotion->claimable_until = time.ToDoubleT();
      }
    }

    auto* public_keys = item.FindListKey("publicKeys");
    if (!public_keys || public_keys->GetListDeprecated().empty()) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }

    std::string keys_json;
    base::JSONWriter::Write(*public_keys, &keys_json);
    promotion->public_keys = keys_json;

    auto legacy_claimed = item.FindBoolKey("legacyClaimed");
    promotion->legacy_claimed = legacy_claimed.value_or(false);

    list->push_back(std::move(promotion));
  }

  if (promotion_size != list->size()) {
    return type::Result::CORRUPTED_DATA;
  }

  return type::Result::LEDGER_OK;
}

void GetAvailable::Request(
    const std::string& platform,
    GetAvailableCallback callback) {
  auto url_callback = std::bind(&GetAvailable::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl(platform);
  ledger_->LoadURL(std::move(request), url_callback);
}

void GetAvailable::OnRequest(
    const type::UrlResponse& response,
    GetAvailableCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  type::PromotionList list;
  std::vector<std::string> corrupted_promotions;
  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result, std::move(list), corrupted_promotions);
    return;
  }

  result = ParseBody(response.body, &list, &corrupted_promotions);
  callback(result, std::move(list), corrupted_promotions);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
