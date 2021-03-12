/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_

#include <memory>
#include <string>

#include "bat/ads/internal/ad_targeting/resources/resource.h"
#include "bat/ads/internal/ml/pipeline/text_processing/text_processing.h"

namespace ads {
namespace ad_targeting {
namespace resource {

class TextClassification : public Resource<ml::pipeline::TextProcessing*> {
 public:
  TextClassification();

  ~TextClassification() override;

  bool IsInitialized() const override;

  void LoadForLocale(const std::string& locale);

  void LoadForId(const std::string& locale);

  ml::pipeline::TextProcessing* get() const override;

 private:
  std::unique_ptr<ml::pipeline::TextProcessing> text_processing_pipeline_;
};

}  // namespace resource
}  // namespace ad_targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
