/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/publisher_info.h"
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

}  // namespace braveledger_bind_util
