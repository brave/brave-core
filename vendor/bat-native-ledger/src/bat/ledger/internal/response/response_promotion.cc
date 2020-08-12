/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/response/response_promotion.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ledger/internal/logging.h"
#include "bat/ledger/internal/promotion/promotion_util.h"
#include "net/http/http_status_code.h"

namespace braveledger_response_util {

// Request Url:
// POST /v1/promotions/{promotion_id}
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "claimId": "53714048-9675-419e-baa3-369d85a2facb"
// }

ledger::Result ParseClaimCreds(
    const ledger::UrlResponse& response,
    std::string* claim_id) {
  DCHECK(claim_id);

  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Forbidden (403)
  if (response.status_code == net::HTTP_FORBIDDEN) {
    BLOG(0, "Signature validation failed");
    return ledger::Result::LEDGER_ERROR;
  }

  // Conflict (409)
  if (response.status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Incorrect blinded credentials");
    return ledger::Result::LEDGER_ERROR;
  }

  // Gone (410)
  if (response.status_code == net::HTTP_GONE) {
    BLOG(0, "Promotion is gone");
    return ledger::Result::NOT_FOUND;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  auto* id = dictionary->FindStringKey("claimId");
  if (!id || id->empty()) {
    BLOG(0, "Claim id is missing");
    return ledger::Result::LEDGER_ERROR;
  }

  *claim_id = *id;

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// GET /v1/promotions?migrate=true&paymentId={payment_id}&platform={platform}
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "promotions": [
//     {
//       "id": "83b3b77b-e7c3-455b-adda-e476fa0656d2",
//       "createdAt": "2020-06-08T15:04:45.352584Z",
//       "expiresAt": "2020-10-08T15:04:45.352584Z",
//       "version": 5,
//       "suggestionsPerGrant": 120,
//       "approximateValue": "30",
//       "type": "ugp",
//       "available": true,
//       "platform": "desktop",
//       "publicKeys": [
//         "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I="
//       ],
//       "legacyClaimed": false
//     }
//   ]
// }

ledger::Result CheckFetchPromotions(const ledger::UrlResponse& response) {
  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid paymentId or platform in request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Not Found (404)
  if (response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized paymentId/promotion combination");
    return ledger::Result::NOT_FOUND;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result ParseFetchPromotions(
    const ledger::UrlResponse& response,
    ledger::PromotionList* list,
    std::vector<std::string>* corrupted_promotions) {
  DCHECK(list && corrupted_promotions);

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  auto* promotions = dictionary->FindListKey("promotions");
  if (!promotions) {
    return ledger::Result::LEDGER_OK;
  }

  const auto promotion_size = promotions->GetList().size();
  for (auto& item : promotions->GetList()) {
    ledger::PromotionPtr promotion = ledger::Promotion::New();

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
        braveledger_promotion::ConvertStringToPromotionType(*type);

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
      promotion->status = ledger::PromotionStatus::ACTIVE;
    } else {
      promotion->status = ledger::PromotionStatus::OVER;
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

    auto* public_keys = item.FindListKey("publicKeys");
    if (!public_keys || public_keys->GetList().empty()) {
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
    return ledger::Result::CORRUPTED_DATA;
  }

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// POST /v1/promotions/reportclobberedclaims
//
// Success:
// OK (200)
//
// Response Format:
// {Empty body}

ledger::Result CheckCorruptedPromotions(const ledger::UrlResponse& response) {
  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// POST /v1/suggestions
//
// Success:
// OK (200)
//
// Response Format:
// {Empty body}

ledger::Result CheckRedeemTokens(const ledger::UrlResponse& response) {
  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

}  // namespace braveledger_response_util
