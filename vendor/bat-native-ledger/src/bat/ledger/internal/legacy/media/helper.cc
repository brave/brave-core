/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/legacy/media/helper.h"

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "bat/ledger/internal/legacy/bat_helper.h"

namespace braveledger_media {

std::string GetMediaKey(const std::string& mediaId, const std::string& type) {
  if (mediaId.empty() || type.empty()) {
    return std::string();
  }

  return type + "_" + mediaId;
}

void GetTwitchParts(
    const std::string& query,
    std::vector<base::flat_map<std::string, std::string>>* parts) {
  size_t pos = query.find("data=");

  if (std::string::npos == pos || query.length() <= 5) {
    return;
  }

  std::string varValue = query.substr(5);
  std::string decoded;
  bool succeded = base::Base64Decode(varValue, &decoded);
  if (succeded) {
    braveledger_bat_helper::getJSONTwitchProperties(decoded, parts);
  }
}

// static
std::string ExtractData(const std::string& data,
                        const std::string& match_after,
                        const std::string& match_until) {
  std::string match;
  size_t match_after_size = match_after.size();
  size_t data_size = data.size();

  if (data_size < match_after_size) {
    return match;
  }

  size_t start_pos = data.find(match_after);
  if (start_pos != std::string::npos) {
    start_pos += match_after_size;
    size_t endPos = data.find(match_until, start_pos);
    if (endPos != start_pos) {
      if (endPos != std::string::npos && endPos > start_pos) {
        match = data.substr(start_pos, endPos - start_pos);
      } else if (endPos != std::string::npos) {
        match = data.substr(start_pos, endPos);
      } else {
        match = data.substr(start_pos, std::string::npos);
      }
    } else if (match_until.empty()) {
      match = data.substr(start_pos, std::string::npos);
    }
  }

  return match;
}

void GetVimeoParts(
    const std::string& query,
    std::vector<base::flat_map<std::string, std::string>>* parts) {
  absl::optional<base::Value> data = base::JSONReader::Read(query);
  if (!data || !data->is_list()) {
    return;
  }

  for (const auto& item : data->GetListDeprecated()) {
    if (item.is_dict()) {
      base::flat_map<std::string, std::string> part;
      const auto* name = item.FindStringKey("name");
      if (name) {
        part.emplace("event", *name);
      }

      const auto clip_id = item.FindIntKey("clip_id");
      if (clip_id) {
        part.emplace("video_id", std::to_string(*clip_id));
      }

      const auto* product = item.FindStringKey("product");
      if (product) {
        part.emplace("type", *product);
      }

      const auto video_time = item.FindDoubleKey("video_time");
      if (video_time) {
        part.emplace("time", std::to_string(*video_time));
      }

      parts->push_back(part);
    }
  }
}

}  // namespace braveledger_media
