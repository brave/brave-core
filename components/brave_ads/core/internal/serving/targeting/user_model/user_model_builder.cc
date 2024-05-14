/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_builder.h"

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_segments.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/interest/interest_segments.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_segments.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"

namespace brave_ads {

UserModelInfo BuildUserModel() {
  UserModelInfo user_model;

  user_model.intent.segments = BuildIntentSegments();
  user_model.latent_interest.segments = BuildLatentInterestSegments();
  user_model.interest.segments = BuildInterestSegments();

  return user_model;
}

}  // namespace brave_ads
