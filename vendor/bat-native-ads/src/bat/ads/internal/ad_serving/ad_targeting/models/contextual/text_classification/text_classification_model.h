/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_MODEL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_MODEL_H_

#include "bat/ads/internal/ad_serving/ad_targeting/models/model.h"

namespace ads {
namespace ad_targeting {
namespace model {

class TextClassification : public Model {
 public:
  TextClassification();

  ~TextClassification() override;

  SegmentList GetSegments() const override;
};

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_MODEL_H_
