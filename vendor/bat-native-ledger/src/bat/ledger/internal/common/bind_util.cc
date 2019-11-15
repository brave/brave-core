/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/common/bind_util.h"

namespace braveledger_bind_util {

std::string FromContributionQueueToString(ledger::ContributionQueuePtr info) {
  base::Value publishers(base::Value::Type::LIST);
  for (auto& item : info->publishers) {
    base::Value publisher(base::Value::Type::DICTIONARY);
    publisher.SetStringKey("publisher_key", item->publisher_key);
    publisher.SetStringKey("amount_percent",
        std::to_string(item->amount_percent));
    publishers.GetList().push_back(std::move(publisher));
  }

  base::Value queue(base::Value::Type::DICTIONARY);

  queue.SetStringKey("id", std::to_string(info->id));
  queue.SetIntKey("type", static_cast<int>(info->type));
  queue.SetStringKey("amount", std::to_string(info->amount));
  queue.SetBoolKey("partial", info->partial);
  queue.SetKey("publishers", std::move(publishers));

  std::string json;
  base::JSONWriter::Write(queue, &json);

  return json;
}

ledger::ContributionQueuePtr FromStringToContributionQueue(
    const std::string& data) {
  auto queue = ledger::ContributionQueue::New();
  base::Optional<base::Value> value = base::JSONReader::Read(data);

  if (!value || !value->is_dict()) {
    return nullptr;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return nullptr;
  }

  auto* id = dictionary->FindKey("id");
  if (id && id->is_string()) {
    queue->id = std::stoull(id->GetString());
  }

  auto* type = dictionary->FindKey("type");
  if (type && type->is_int()) {
    queue->type = static_cast<ledger::RewardsType>(type->GetInt());
  }

  auto* amount = dictionary->FindKey("amount");
  if (amount && amount->is_string()) {
    queue->amount = std::stod(amount->GetString());
  }

  auto* partial = dictionary->FindKey("partial");
  if (partial && partial->is_bool()) {
    queue->partial = partial->GetBool();
  }

  auto* publishers = dictionary->FindKey("publishers");
  if (publishers && publishers->is_list()) {
    base::ListValue publishers_list(publishers->GetList());
    for (auto& item : publishers_list) {
      auto publisher = ledger::ContributionQueuePublisher::New();
      auto* publisher_key = item.FindKey("publisher_key");
      if (!publisher_key || !publisher_key->is_string()) {
        continue;
      }
      publisher->publisher_key = publisher_key->GetString();

      auto* amount_percent = item.FindKey("amount_percent");
      if (amount_percent && amount_percent->is_string()) {
        publisher->amount_percent = std::stod(amount_percent->GetString());
      }
      queue->publishers.push_back(std::move(publisher));
    }
  }

  return queue;
}

std::string FromPromotionToString(const ledger::PromotionPtr info) {
  base::Value credentials(base::Value::Type::DICTIONARY);
  if (info->credentials) {
    credentials.SetStringKey("tokens", info->credentials->tokens);
    credentials.SetStringKey("blinded_creds", info->credentials->blinded_creds);
    credentials.SetStringKey("signed_creds", info->credentials->signed_creds);
    credentials.SetStringKey("public_key", info->credentials->public_key);
    credentials.SetStringKey("batch_proof", info->credentials->batch_proof);
    credentials.SetStringKey("claim_id", info->credentials->claim_id);
  }

  base::Value promotion(base::Value::Type::DICTIONARY);
  promotion.SetStringKey("id", info->id);
  promotion.SetStringKey("public_keys", info->public_keys);
  promotion.SetStringKey("approximate_value",
      std::to_string(info->approximate_value));
  promotion.SetStringKey("expires_at", std::to_string(info->expires_at));
  promotion.SetIntKey("version", info->version);
  promotion.SetIntKey("type", static_cast<int>(info->type));
  promotion.SetIntKey("suggestions", info->suggestions);
  promotion.SetBoolKey("status", static_cast<int>(info->status));
  promotion.SetKey("credentials", std::move(credentials));

  std::string json;
  base::JSONWriter::Write(promotion, &json);

  return json;
}

ledger::PromotionPtr FromStringToPromotion(const std::string& data) {
  auto promotion = ledger::Promotion::New();
  base::Optional<base::Value> value = base::JSONReader::Read(data);

  if (!value || !value->is_dict()) {
    return nullptr;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return nullptr;
  }

  auto* id = dictionary->FindStringKey("id");
  if (id) {
    promotion->id = *id;
  }

  auto* public_keys = dictionary->FindStringKey("public_keys");
  if (public_keys) {
    promotion->public_keys = *public_keys;
  }

  auto* approximate_value = dictionary->FindStringKey("approximate_value");
  if (approximate_value) {
    promotion->approximate_value = std::stod(*approximate_value);
  }

  auto* expires_at = dictionary->FindStringKey("expires_at");
  if (expires_at) {
    promotion->expires_at = std::stoull(*expires_at);
  }

  auto version = dictionary->FindIntKey("version");
  if (version) {
    promotion->version = *version;
  }

  auto type = dictionary->FindIntKey("type");
  if (type) {
    promotion->type = static_cast<ledger::PromotionType>(*type);
  }

  auto suggestions = dictionary->FindIntKey("suggestions");
  if (suggestions) {
    promotion->suggestions = *suggestions;
  }

  auto status = dictionary->FindIntKey("status");
  if (status) {
    promotion->status = static_cast<ledger::PromotionStatus>(*status);
  }

  auto* credentials = dictionary->FindDictKey("credentials");
  if (credentials) {
    auto creds = ledger::PromotionCreds::New();

    auto* tokens = credentials->FindStringKey("tokens");
    if (tokens) {
      creds->tokens = *tokens;
    }

    auto* blinded_creds = credentials->FindStringKey("blinded_creds");
    if (blinded_creds) {
      creds->blinded_creds = *blinded_creds;
    }

    auto* signed_creds = credentials->FindStringKey("signed_creds");
    if (signed_creds) {
      creds->signed_creds = *signed_creds;
    }

    auto* public_key = credentials->FindStringKey("public_key");
    if (public_key) {
      creds->public_key = *public_key;
    }

    auto* batch_proof = credentials->FindStringKey("batch_proof");
    if (batch_proof) {
      creds->batch_proof = *batch_proof;
    }

    auto* claim_id = credentials->FindStringKey("claim_id");
    if (claim_id) {
      creds->claim_id = *claim_id;
    }

    promotion->credentials = std::move(creds);
  }

  return promotion;
}

}  // namespace braveledger_bind_util
