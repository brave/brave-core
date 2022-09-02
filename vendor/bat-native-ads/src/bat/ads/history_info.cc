/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/history_info.h"

#include "absl/types/optional.h"
#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ads/history_item_info.h"

namespace ads {

HistoryInfo::HistoryInfo() = default;

HistoryInfo::HistoryInfo(const HistoryInfo& info) = default;

HistoryInfo& HistoryInfo::operator=(const HistoryInfo& info) = default;

HistoryInfo::~HistoryInfo() = default;

base::Value::Dict HistoryInfo::ToValue() const {
  base::Value::List history;
  for (const auto& item : items) {
    history.Append(item.ToValue());
  }

  base::Value::Dict dict;
  dict.Set("history", std::move(history));
  return dict;
}

void HistoryInfo::FromValue(const base::Value::Dict& root) {
  if (const auto* value = root.FindList("history")) {
    for (const auto& item : *value) {
      if (!item.is_dict()) {
        continue;
      }

      HistoryItemInfo history_item;
      history_item.FromValue(item.GetDict());
      items.push_back(history_item);
    }
  }
}

std::string HistoryInfo::ToJson() const {
  std::string json;
  CHECK(base::JSONWriter::Write(ToValue(), &json));
  return json;
}

bool HistoryInfo::FromJson(const std::string& json) {
  const absl::optional<base::Value> root =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);

  if (!root || !root->is_dict()) {
    return false;
  }

  FromValue(root->GetDict());

  return true;
}

}  // namespace ads
