/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/legacy/media/helper.h"

namespace brave_rewards::internal {

std::string GetMediaKey(const std::string& mediaId, const std::string& type) {
  if (mediaId.empty() || type.empty()) {
    return std::string();
  }

  return type + "_" + mediaId;
}

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

}  // namespace brave_rewards::internal
