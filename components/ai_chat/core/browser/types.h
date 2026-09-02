/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TYPES_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TYPES_H_

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"
#include "url/origin.h"

namespace ai_chat {

using ConversationCapabilitySet =
    absl::flat_hash_set<mojom::ConversationCapability>;

struct SearchQuerySummary {
  std::string query;
  std::string summary;

  bool operator==(const SearchQuerySummary& other) const = default;
};

struct Tab {
  std::string id;
  std::string title;
  url::Origin origin;
  // Excerpts of the page's text, read from the history embeddings store.
  // Empty when the page has no indexed content or the user hasn't enabled
  // history embeddings.
  std::vector<std::string> passages;

  bool operator==(const Tab& other) const = default;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TYPES_H_
