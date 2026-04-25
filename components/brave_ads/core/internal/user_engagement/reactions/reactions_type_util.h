/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_REACTIONS_REACTIONS_TYPE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_REACTIONS_REACTIONS_TYPE_UTIL_H_

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

mojom::ReactionType ToggleLikedReactionType(
    mojom::ReactionType mojom_reaction_type);

mojom::ReactionType ToggleDislikedReactionType(
    mojom::ReactionType mojom_reaction_type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_REACTIONS_REACTIONS_TYPE_UTIL_H_
