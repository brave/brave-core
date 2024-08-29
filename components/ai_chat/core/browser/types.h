/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TYPES_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TYPES_H_

#include <string>

namespace ai_chat {

struct SearchQuerySummary {
  std::string query;
  std::string summary;

  SearchQuerySummary(const std::string& query, const std::string& summary)
      : query(query), summary(summary) {}

  bool operator==(const SearchQuerySummary& other) const {
    return query == other.query && summary == other.summary;
  }
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TYPES_H_
