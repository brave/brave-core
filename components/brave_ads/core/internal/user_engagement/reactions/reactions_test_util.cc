/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions_test_util.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::test {

mojom::ReactionInfoPtr BuildReaction(const mojom::AdType ad_type) {
  mojom::ReactionInfoPtr mojom_reaction = mojom::ReactionInfo::New();

  mojom_reaction->ad_type = ad_type;
  mojom_reaction->creative_instance_id = test::kCreativeInstanceId;
  mojom_reaction->creative_set_id = test::kCreativeSetId;
  mojom_reaction->advertiser_id = test::kAdvertiserId;
  mojom_reaction->segment = test::kSegment;

  return mojom_reaction;
}

}  // namespace brave_ads::test
