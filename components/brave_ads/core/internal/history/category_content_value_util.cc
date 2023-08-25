/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/category_content_value_util.h"

#include "brave/components/brave_ads/core/public/history/category_content_info.h"

namespace brave_ads {

namespace {

constexpr char kCategoryKey[] = "category";
constexpr char kUserReactionTypeKey[] = "optAction";

constexpr char kLegacyUserReactionTypeKey[] = "opt_action";

}  // namespace

base::Value::Dict CategoryContentToValue(
    const CategoryContentInfo& category_content) {
  return base::Value::Dict()
      .Set(kCategoryKey, category_content.category)
      .Set(kUserReactionTypeKey,
           static_cast<int>(category_content.user_reaction_type));
}

CategoryContentInfo CategoryContentFromValue(const base::Value::Dict& dict) {
  CategoryContentInfo category_content;

  if (const auto* value = dict.FindString(kCategoryKey)) {
    category_content.category = *value;
  }

  if (const auto user_reaction_type = dict.FindInt(kUserReactionTypeKey)) {
    category_content.user_reaction_type =
        static_cast<mojom::UserReactionType>(*user_reaction_type);
  } else if (const auto legacy_user_reaction_type =
                 dict.FindInt(kLegacyUserReactionTypeKey)) {
    category_content.user_reaction_type =
        static_cast<mojom::UserReactionType>(*legacy_user_reaction_type);
  }

  return category_content;
}

}  // namespace brave_ads
