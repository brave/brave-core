/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/user_engagement/reactions/reactions_util.h"

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

mojom::ReactionInfoPtr CreateReaction(
    const AdHistoryItemInfo& ad_history_item) {
  mojom::ReactionInfoPtr mojom_reaction = mojom::ReactionInfo::New();

  mojom_reaction->ad_type = static_cast<mojom::AdType>(ad_history_item.type);
  mojom_reaction->creative_instance_id = ad_history_item.creative_instance_id;
  mojom_reaction->creative_set_id = ad_history_item.creative_set_id;
  mojom_reaction->advertiser_id = ad_history_item.advertiser_id;
  mojom_reaction->segment = ad_history_item.segment;

  return mojom_reaction;
}

}  // namespace brave_ads
