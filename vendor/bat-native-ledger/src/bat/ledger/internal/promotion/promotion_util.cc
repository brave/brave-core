/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/promotion/promotion_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"

namespace braveledger_promotion {

std::string ParseOSToString(ledger::OperatingSystem os) {
  switch (static_cast<int>(os)) {
    case static_cast<int>(ledger::OperatingSystem::WINDOWS):  {
      return "windows";
    }
    case static_cast<int>(ledger::OperatingSystem::MACOS):  {
      return "osx";
    }
    case static_cast<int>(ledger::OperatingSystem::LINUX):  {
      return "linux";
    }
    case static_cast<int>(ledger::OperatingSystem::UNDEFINED):  {
      return "undefined";
    }
    default: {
      NOTREACHED();
      return "";
    }
  }
}

std::string ParseClientInfoToString(ledger::ClientInfoPtr info) {
  if (!info) {
    return "";
  }

  switch (static_cast<int>(info->platform)) {
    case static_cast<int>(ledger::Platform::ANDROID_R):  {
      return "android";
    }
    case static_cast<int>(ledger::Platform::IOS):  {
      return "ios";
    }
    case static_cast<int>(ledger::Platform::DESKTOP):  {
      return ParseOSToString(info->os);
    }
    default: {
      NOTREACHED();
      return "";
    }
  }
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

ledger::ReportType ConvertPromotionTypeToReportType(
    const ledger::PromotionType type) {
  switch (static_cast<int>(type)) {
    case static_cast<int>(ledger::PromotionType::UGP): {
      return ledger::ReportType::GRANT;
    }
    case static_cast<int>(ledger::PromotionType::ADS): {
      return ledger::ReportType::ADS;
    }
    default: {
      NOTREACHED();
      return ledger::ReportType::GRANT;
    }
  }
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
    const auto promotion_size = promotions->GetList().size();
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
      promotion->approximate_value = std::stod(approximate_value->GetString());

      auto* available = item.FindKey("available");
      if (!available || !available->is_bool()) {
        continue;
      }
      if (available->GetBool()) {
        promotion->status = ledger::PromotionStatus::ACTIVE;
      } else {
        promotion->status = ledger::PromotionStatus::OVER;
      }

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

      auto* public_keys = item.FindListKey("publicKeys");
      if (!public_keys) {
        continue;
      }

      std::string keys_json;
      base::JSONWriter::Write(*public_keys, &keys_json);
      promotion->public_keys = keys_json;

      list->push_back(std::move(promotion));
    }

    if (promotion_size != list->size()) {
      return false;
    }
  }

  return true;
}

std::string ParseClaimTokenResponse(
    const std::string& response) {
  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return "";
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return "";
  }

  auto* id = dictionary->FindStringKey("claimId");
  if (!id) {
    return "";
  }

  return *id;
}

void ParseSignedTokensResponse(
    const std::string& response,
    base::Value* result) {
  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return;
  }

  auto* batch_proof = dictionary->FindStringKey("batchProof");
  if (!batch_proof) {
    return;
  }

  auto* signed_creds = dictionary->FindListKey("signedCreds");
  if (!signed_creds) {
    return;
  }

  auto* public_key = dictionary->FindStringKey("publicKey");
  if (!public_key) {
    return;
  }

  result->SetStringKey("batch_proof", *batch_proof);
  result->SetStringKey("public_key", *public_key);
  result->SetKey("signed_creds", base::Value(signed_creds->GetList()));
}

std::unique_ptr<base::ListValue> ParseStringToBaseList(
    const std::string& string_list) {
  base::Optional<base::Value> value = base::JSONReader::Read(string_list);
  if (!value || !value->is_list()) {
    return std::make_unique<base::ListValue>();
  }

  return std::make_unique<base::ListValue>(value->GetList());
}

bool UnBlindTokensMock(
    ledger::PromotionPtr promotion,
    std::vector<std::string>* unblinded_encoded_tokens) {
  if (!promotion || !promotion->credentials || !unblinded_encoded_tokens) {
    return false;
  }

  auto signed_tokens_base64 = ParseStringToBaseList(
      promotion->credentials->signed_creds);

  for (auto& item : *signed_tokens_base64) {
    unblinded_encoded_tokens->push_back(item.GetString());
  }

  return true;
}

bool VerifyPublicKey(const ledger::PromotionPtr promotion) {
  if (!promotion || !promotion->credentials) {
    return false;
  }

  auto promotion_keys = ParseStringToBaseList(promotion->public_keys);

  if (!promotion_keys) {
    return false;
  }


  for (auto& item : *promotion_keys) {
    if (item.GetString() == promotion->credentials->public_key) {
      return true;
    }
  }

  return false;
}

}  // namespace braveledger_promotion
