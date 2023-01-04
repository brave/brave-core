/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "bat/ads/internal/resources/parsing_result.h"

namespace ads {

namespace ml::pipeline {
class TextProcessing;
}  // namespace ml::pipeline

namespace resource {

class TextClassification final {
 public:
  TextClassification();

  TextClassification(const TextClassification& other) = delete;
  TextClassification& operator=(const TextClassification& other) = delete;

  TextClassification(TextClassification&& other) noexcept = delete;
  TextClassification& operator=(TextClassification&& other) noexcept = delete;

  ~TextClassification();

  bool IsInitialized() const;

  void Load();

  ml::pipeline::TextProcessing* Get() const;

 private:
  void OnLoadAndParseResource(
      ParsingResultPtr<ml::pipeline::TextProcessing> result);

  std::unique_ptr<ml::pipeline::TextProcessing> text_processing_pipeline_;

  base::WeakPtrFactory<TextClassification> weak_ptr_factory_{this};
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
