/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions_type_util.h"

namespace brave_ads {

mojom::ReactionType ToggleLikedReactionType(
    const mojom::ReactionType reaction_type) {
  return reaction_type == mojom::ReactionType::kNeutral
             ? mojom::ReactionType::kLiked
             : mojom::ReactionType::kNeutral;
}

mojom::ReactionType ToggleDislikedReactionType(
    const mojom::ReactionType reaction_type) {
  return reaction_type == mojom::ReactionType::kNeutral
             ? mojom::ReactionType::kDisliked
             : mojom::ReactionType::kNeutral;
}

}  // namespace brave_ads
