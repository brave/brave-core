/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_  // NOLINT

#include <string>

#include "bat/ads/internal/ad_targeting/processors/processor.h"
#include "bat/ads/internal/ad_targeting/resources/text_classification/text_classification_resource.h"

namespace usermodel {
class UserModel;
}  // namespace usermodel

namespace ads {
namespace ad_targeting {
namespace processor {

class TextClassification : public Processor<std::string> {
 public:
  TextClassification(
      resource::TextClassification* resource);

  ~TextClassification() override;

  void Process(
      const std::string& text) override;

 private:
  resource::TextClassification* resource_;
};

}  // namespace processor
}  // namespace ad_targeting
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_  // NOLINT
