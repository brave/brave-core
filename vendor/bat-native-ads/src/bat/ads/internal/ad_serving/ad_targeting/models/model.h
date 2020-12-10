/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_AD_TARGETING_MODEL_H_
#define BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_AD_TARGETING_MODEL_H_

#include "bat/ads/internal/ad_targeting/ad_targeting_aliases.h"

namespace ads {
namespace ad_targeting {
namespace model {

class Model {
 public:
  virtual ~Model() = default;

  virtual SegmentList GetSegments() const = 0;
};

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_AD_TARGETING_MODEL_H_
