/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_news/common/features.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_news::features {

BASE_FEATURE(kBraveNewsCardPeekFeature,
             "BraveNewsCardPeek",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveNewsFeedUpdate,
             "BraveNewsFeedUpdate",
             base::FEATURE_DISABLED_BY_DEFAULT);
const base::FeatureParam<int> kBraveNewsMinBlockCards{&kBraveNewsFeedUpdate,
                                                      "min-block-cards", 1};

const base::FeatureParam<int> kBraveNewsMaxBlockCards{&kBraveNewsFeedUpdate,
                                                      "max-block-cards", 5};

const base::FeatureParam<double> kBraveNewsPopScoreHalfLife{
    &kBraveNewsFeedUpdate, "pop-score-half-life", 18};
const base::FeatureParam<double> kBraveNewsPopScoreFallback{
    &kBraveNewsFeedUpdate, "pop-score-fallback", 50};

const base::FeatureParam<double> kBraveNewsInlineDiscoveryRatio{
    &kBraveNewsFeedUpdate, "inline-discovery-ratio", 0.25};

const base::FeatureParam<double> kBraveNewsSourceSubscribedMin{
    &kBraveNewsFeedUpdate, "source-subscribed-min", 1e-5};
const base::FeatureParam<double> kBraveNewsSourceSubscribedBoost{
    &kBraveNewsFeedUpdate, "source-subscribed-boost", 1};
const base::FeatureParam<double> kBraveNewsChannelSubscribedBoost{
    &kBraveNewsFeedUpdate, "channel-subscribed-boost", 0.2};

const base::FeatureParam<double> kBraveNewsSourceVisitsMin{
    &kBraveNewsFeedUpdate, "source-visits-min", 0.2};

const base::FeatureParam<double> kBraveNewsCategoryTopicRatio{
    &kBraveNewsFeedUpdate, "category-topic-ratio", 0.5};

}  // namespace brave_news::features
