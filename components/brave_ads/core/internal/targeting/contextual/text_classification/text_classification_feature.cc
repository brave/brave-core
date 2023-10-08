/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"  // IWYU pragma: keep

namespace brave_ads {

BASE_FEATURE(kTextClassificationFeature,
             "TextClassification",
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace brave_ads
