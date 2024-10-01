/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions_type_util.h"

#include "base/notreached.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

mojom::ReactionType ToggleLikedReactionType(
    const mojom::ReactionType mojom_reaction_type) {
  switch (mojom_reaction_type) {
    case mojom::ReactionType::kNeutral:
    case mojom::ReactionType::kDisliked: {
      // If the reaction is neutral or disliked, toggle to liked.
      return mojom::ReactionType::kLiked;
    }

    case mojom::ReactionType::kLiked: {
      // If the reaction is liked, toggle to neutral.
      return mojom::ReactionType::kNeutral;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for mojom::ReactionType: "
                        << mojom_reaction_type;
}

mojom::ReactionType ToggleDislikedReactionType(
    const mojom::ReactionType mojom_reaction_type) {
  switch (mojom_reaction_type) {
    case mojom::ReactionType::kNeutral:
    case mojom::ReactionType::kLiked: {
      // If the reaction is neutral or liked, toggle to disliked.
      return mojom::ReactionType::kDisliked;
    }

    case mojom::ReactionType::kDisliked: {
      // If the reaction is disliked, toggle to neutral.
      return mojom::ReactionType::kNeutral;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for mojom::ReactionType: "
                        << mojom_reaction_type;
}

}  // namespace brave_ads
