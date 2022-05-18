/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_PROCESSORS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_PROCESSORS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "bat/ads/internal/targeting/processors/processor_interface.h"

namespace ads {

namespace resource {
class TextClassification;
}  // namespace resource

namespace targeting {
namespace processor {

class TextClassification final : public ProcessorInterface<std::string> {
 public:
  explicit TextClassification(resource::TextClassification* resource);
  ~TextClassification() override;

  void Process(const std::string& text) override;

 private:
  raw_ptr<resource::TextClassification> resource_ = nullptr;
};

}  // namespace processor
}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_PROCESSORS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_
