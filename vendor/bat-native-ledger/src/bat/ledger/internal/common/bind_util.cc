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
    publishers.Append(std::move(publisher));
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
  promotion.SetStringKey("claimed_at", std::to_string(info->claimed_at));
  promotion.SetIntKey("version", info->version);
  promotion.SetIntKey("type", static_cast<int>(info->type));
  promotion.SetIntKey("suggestions", info->suggestions);
  promotion.SetIntKey("status", static_cast<int>(info->status));
  promotion.SetKey("credentials", std::move(credentials));
  promotion.SetBoolKey("legacy_claimed", info->legacy_claimed);

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

  auto* claimed_at = dictionary->FindStringKey("claimed_at");
  if (claimed_at) {
    promotion->claimed_at = std::stoull(*claimed_at);
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

  auto legacy_claimed = dictionary->FindBoolKey("legacy_claimed");
  if (legacy_claimed) {
    promotion->legacy_claimed = *legacy_claimed;
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

std::string FromContributionToString(const ledger::ContributionInfoPtr info) {
  if (!info) {
    return "{}";
  }

  base::Value publishers(base::Value::Type::LIST);
  for (auto& item : info->publishers) {
    base::Value publisher(base::Value::Type::DICTIONARY);
    publisher.SetStringKey("contribution_id", item->contribution_id);
    publisher.SetStringKey("publisher_key", item->publisher_key);
    publisher.SetDoubleKey("total_amount", item->total_amount);
    publisher.SetDoubleKey("contributed_amount", item->contributed_amount);
    publishers.Append(std::move(publisher));
  }

  base::Value queue(base::Value::Type::DICTIONARY);

  queue.SetStringKey("contribution_id", info->contribution_id);
  queue.SetDoubleKey("amount", info->amount);
  queue.SetIntKey("type", static_cast<int>(info->type));
  queue.SetIntKey("step", static_cast<int>(info->step));
  queue.SetIntKey("retry_count", info->retry_count);
  queue.SetStringKey("created_at", std::to_string(info->created_at));
  queue.SetIntKey("processor", static_cast<int>(info->processor));
  queue.SetKey("publishers", std::move(publishers));

  std::string json;
  base::JSONWriter::Write(queue, &json);

  return json;
}

ledger::ContributionInfoPtr FromStringToContribution(const std::string& data) {
  base::Optional<base::Value> value = base::JSONReader::Read(data);

  if (!value || !value->is_dict()) {
    return nullptr;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return nullptr;
  }

  auto contribution = ledger::ContributionInfo::New();

  std::string id;
  const auto* contribution_id = dictionary->FindStringKey("contribution_id");
  if (contribution_id) {
    contribution->contribution_id = *contribution_id;
    id = contribution->contribution_id;
  }

  if (id.empty()) {
    return nullptr;
  }

  const auto amount = dictionary->FindDoubleKey("amount");
  if (amount) {
    contribution->amount = *amount;
  }

  const auto type = dictionary->FindIntKey("type");
  if (type) {
    contribution->type = static_cast<ledger::RewardsType>(*type);
  }

  const auto step = dictionary->FindIntKey("step");
  if (step) {
    contribution->step = static_cast<ledger::ContributionStep>(*step);
  }

  const auto retry_count = dictionary->FindIntKey("retry_count");
  if (retry_count) {
    contribution->retry_count = *retry_count;
  }

  const auto* created_at = dictionary->FindStringKey("created_at");
  if (created_at) {
    contribution->created_at = std::stoull(*created_at);
  }

  const auto processor = dictionary->FindIntKey("processor");
  if (processor) {
    contribution->processor =
        static_cast<ledger::ContributionProcessor>(*processor);
  }

  auto* publishers = dictionary->FindListKey("publishers");
  if (publishers) {
    for (auto& item : publishers->GetList()) {
      auto publisher = ledger::ContributionPublisher::New();
      publisher->contribution_id = id;

      const auto* publisher_key = item.FindStringKey("publisher_key");
      if (publisher_key) {
        publisher->publisher_key = *publisher_key;
      }

      const auto total_amount = item.FindDoubleKey("total_amount");
      if (total_amount) {
        publisher->total_amount = *total_amount;
      }

      const auto contributed_amount = item.FindDoubleKey("contributed_amount");
      if (contributed_amount) {
        publisher->contributed_amount = *contributed_amount;
      }

      contribution->publishers.push_back(std::move(publisher));
    }
  }

  return contribution;
}

std::string FromContributionListToString(ledger::ContributionInfoList list) {
  base::Value items(base::Value::Type::LIST);

  for (auto& contribution : list) {
    items.Append(
        base::Value(FromContributionToString(std::move(contribution))));
  }

  std::string json;
  base::JSONWriter::Write(items, &json);

  return json;
}

void FromStringToContributionList(
    const std::string& data,
    ledger::ContributionInfoList* contribution_list) {
  DCHECK(contribution_list);

  base::Optional<base::Value> list = base::JSONReader::Read(data);
  if (!list || !list->is_list()) {
    return;
  }

  base::ListValue* list_value = nullptr;
  if (!list->GetAsList(&list_value)) {
    return;
  }

  for (auto& item : list_value->GetList()) {
    if (!item.is_string()) {
      continue;
    }

    auto info = FromStringToContribution(item.GetString());
    if (!info) {
      continue;
    }

    contribution_list->push_back(std::move(info));
  }
}

std::string FromMonthlyReportToString(ledger::MonthlyReportInfoPtr info) {
  base::Value balance(base::Value::Type::DICTIONARY);
  if (info->balance) {
    balance.SetDoubleKey("grants", info->balance->grants);
    balance.SetDoubleKey("earning_from_ads", info->balance->earning_from_ads);
    balance.SetDoubleKey("auto_contribute", info->balance->auto_contribute);
    balance.SetDoubleKey("recurring_donation",
        info->balance->recurring_donation);
    balance.SetDoubleKey("one_time_donation", info->balance->one_time_donation);
  }

  base::Value transactions(base::Value::Type::LIST);
  for (auto& item : info->transactions) {
    base::Value transaction(base::Value::Type::DICTIONARY);
    transaction.SetDoubleKey("amount", item->amount);
    transaction.SetIntKey("type", static_cast<int>(item->type));
    transaction.SetStringKey("created_at", std::to_string(item->created_at));
    transactions.Append(std::move(transaction));
  }

  base::Value contributions(base::Value::Type::LIST);
  for (auto& item : info->contributions) {
    base::Value publishers(base::Value::Type::LIST);
    for (auto& item_publisher : item->publishers) {
      base::Value publisher(base::Value::Type::DICTIONARY);
      publisher.SetStringKey("id", item_publisher->id);
      publisher.SetDoubleKey("weight", item_publisher->weight);
      publisher.SetStringKey("name", item_publisher->name);
      publisher.SetStringKey("url", item_publisher->url);
      publisher.SetStringKey("favicon_url", item_publisher->favicon_url);
      publisher.SetIntKey("status", static_cast<int>(item_publisher->status));
      publisher.SetStringKey("provider", item_publisher->provider);
    }

    base::Value contribution(base::Value::Type::DICTIONARY);
    contribution.SetStringKey("contribution_id", item->contribution_id);
    contribution.SetDoubleKey("amount", item->amount);
    contribution.SetIntKey("type", static_cast<int>(item->type));
    contribution.SetKey("publishers", std::move(publishers));
    contribution.SetStringKey("created_at", std::to_string(item->created_at));
    contributions.Append(std::move(contribution));
  }

  base::Value monthly(base::Value::Type::DICTIONARY);
  monthly.SetKey("balance", std::move(balance));
  monthly.SetKey("transactions", std::move(transactions));
  monthly.SetKey("contributions", std::move(contributions));

  std::string json;
  base::JSONWriter::Write(monthly, &json);

  return json;
}

ledger::MonthlyReportInfoPtr FromStringToMonthlyReport(
    const std::string& data) {
  base::Optional<base::Value> value = base::JSONReader::Read(data);

  if (!value || !value->is_dict()) {
    return nullptr;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return nullptr;
  }

  auto balance_report = ledger::BalanceReportInfo::New();
  auto* balance = dictionary->FindDictKey("balance");
  if (balance) {
    const auto grants = balance->FindDoubleKey("grants");
    if (grants) {
      balance_report->grants = *grants;
    }

    const auto earning_from_ads = balance->FindDoubleKey("earning_from_ads");
    if (earning_from_ads) {
      balance_report->earning_from_ads = *earning_from_ads;
    }

    const auto auto_contribute = balance->FindDoubleKey("auto_contribute");
    if (auto_contribute) {
      balance_report->auto_contribute = *auto_contribute;
    }

    const auto recurring_donation =
        balance->FindDoubleKey("recurring_donation");
    if (recurring_donation) {
      balance_report->recurring_donation = *recurring_donation;
    }

    const auto one_time_donation = balance->FindDoubleKey("one_time_donation");
    if (one_time_donation) {
      balance_report->one_time_donation = *one_time_donation;
    }
  }

  ledger::TransactionReportInfoList transaction_report;
  auto* transactions = dictionary->FindListKey("transactions");
  if (transactions) {
    for (auto& item : transactions->GetList()) {
      auto transaction = ledger::TransactionReportInfo::New();

      const auto amount = item.FindDoubleKey("amount");
      if (amount) {
        transaction->amount = *amount;
      }

      const auto type = item.FindIntKey("type");
      if (type) {
        transaction->type = static_cast<ledger::ReportType>(*type);
      }

      const auto* created_at = item.FindStringKey("created_at");
      if (created_at) {
        transaction->created_at = std::stoull(*created_at);
      }

      transaction_report.push_back(std::move(transaction));
    }
  }

  ledger::ContributionReportInfoList contribution_report;
  auto* contributions = dictionary->FindListKey("contributions");
  if (contributions) {
    for (auto& item : contributions->GetList()) {
      auto contribution = ledger::ContributionReportInfo::New();

      const auto* contribution_id = item.FindStringKey("contribution_id");
      if (contribution_id) {
        contribution->contribution_id = *contribution_id;
      }

      const auto amount = item.FindDoubleKey("amount");
      if (amount) {
        contribution->amount = *amount;
      }

      const auto type = item.FindIntKey("type");
      if (type) {
        contribution->type = static_cast<ledger::ReportType>(*type);
      }

      const auto* created_at = item.FindStringKey("created_at");
      if (created_at) {
        contribution->created_at = std::stoull(*created_at);
      }

      ledger::PublisherInfoList publisher_list;
      auto* publishers = item.FindListKey("publishers");
      if (publishers) {
        for (auto& item_publisher : publishers->GetList()) {
          auto publisher = ledger::PublisherInfo::New();

          const auto* id = item_publisher.FindStringKey("id");
          if (id) {
            publisher->id = *id;
          }

          const auto weight = item_publisher.FindDoubleKey("weight");
          if (weight) {
            publisher->weight = *weight;
          }

          const auto* name = item_publisher.FindStringKey("name");
          if (name) {
            publisher->name = *name;
          }

          const auto* url = item_publisher.FindStringKey("url");
          if (url) {
            publisher->url = *url;
          }

          const auto* favicon_url = item_publisher.FindStringKey("favicon_url");
          if (favicon_url) {
            publisher->favicon_url = *favicon_url;
          }

          const auto status = item_publisher.FindIntKey("status");
          if (status) {
            publisher->status = static_cast<ledger::PublisherStatus>(*status);
          }

          const auto* provider = item_publisher.FindStringKey("provider");
          if (provider) {
            publisher->provider = *provider;
          }

          publisher_list.push_back(std::move(publisher));
        }
      }

      contribution->publishers = std::move(publisher_list);
      contribution_report.push_back(std::move(contribution));
    }
  }

  auto info = ledger::MonthlyReportInfo::New();
  info->balance = std::move(balance_report);
  info->transactions = std::move(transaction_report);
  info->contributions = std::move(contribution_report);

  return info;
}

}  // namespace braveledger_bind_util
