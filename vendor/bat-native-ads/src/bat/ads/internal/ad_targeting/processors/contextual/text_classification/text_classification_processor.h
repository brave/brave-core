/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_

#include <string>

#include "bat/ads/internal/ad_targeting/processors/processor.h"
#include "bat/ads/internal/ad_targeting/resources/contextual/text_classification/text_classification_resource.h"

namespace ads {
namespace ad_targeting {
namespace processor {

class TextClassification : public Processor<std::string> {
 public:
  explicit TextClassification(resource::TextClassification* resource);

  ~TextClassification() override;

  void Process(const std::string& text) override;

 private:
  resource::TextClassification* resource_;
};

}  // namespace processor
}  // namespace ad_targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_
