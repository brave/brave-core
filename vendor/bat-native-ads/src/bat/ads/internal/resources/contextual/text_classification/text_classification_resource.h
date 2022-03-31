/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_

#include <memory>

#include "bat/ads/internal/resources/resource.h"

namespace ads {

namespace ml {
namespace pipeline {
class TextProcessing;
}  // namespace pipeline
}  // namespace ml

namespace resource {

class TextClassification final
    : public Resource<ml::pipeline::TextProcessing*> {
 public:
  TextClassification();
  ~TextClassification() override;

  bool IsInitialized() const override;

  void Load();

  ml::pipeline::TextProcessing* get() const override;

 private:
  std::unique_ptr<ml::pipeline::TextProcessing> text_processing_pipeline_;
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
