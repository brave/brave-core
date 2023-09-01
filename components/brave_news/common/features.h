/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_news::features {

BASE_DECLARE_FEATURE(kBraveNewsCardPeekFeature);

BASE_DECLARE_FEATURE(kBraveNewsFeedUpdate);
extern const base::FeatureParam<int> kBraveNewsMinBlockCards;
extern const base::FeatureParam<int> kBraveNewsMaxBlockCards;

extern const base::FeatureParam<double> kBraveNewsPopScoreHalfLife;
extern const base::FeatureParam<double> kBraveNewsPopScoreFallback;
}  // namespace brave_news::features

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_FEATURES_H_
