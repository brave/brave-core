/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions_value_util.h"

#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

base::Value::Dict ReactionMapToDict(const ReactionMap& reactions) {
  base::Value::Dict dict;

  for (const auto& [id, type] : reactions) {
    dict.Set(id, static_cast<int>(type));
  }

  return dict;
}

ReactionMap ReactionMapFromDict(const base::Value::Dict& dict) {
  ReactionMap reactions;

  for (const auto [id, value] : dict) {
    if (const std::optional<int> type = value.GetIfInt()) {
      reactions[id] = static_cast<mojom::ReactionType>(*type);
    }
  }

  return reactions;
}

base::Value::List ReactionSetToList(const ReactionSet& reactions) {
  base::Value::List list;

  for (const auto& reaction : reactions) {
    list.Append(base::Value(reaction));
  }

  return list;
}

ReactionSet ReactionSetFromList(const base::Value::List& list) {
  ReactionSet reactions;

  for (const auto& value : list) {
    if (const std::string* const id = value.GetIfString()) {
      reactions.insert(*id);
    }
  }

  return reactions;
}

}  // namespace brave_ads
