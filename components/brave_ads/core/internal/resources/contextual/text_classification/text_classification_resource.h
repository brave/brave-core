/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"
#include "brave/components/brave_ads/core/internal/resources/parsing_error_or.h"

namespace brave_ads::resource {

class TextClassification final {
 public:
  TextClassification();

  TextClassification(const TextClassification&) = delete;
  TextClassification& operator=(const TextClassification&) = delete;

  TextClassification(TextClassification&&) noexcept = delete;
  TextClassification& operator=(TextClassification&&) noexcept = delete;

  ~TextClassification();

  bool IsInitialized() const;

  void Load();

  const ml::pipeline::TextProcessing* Get() const;

 private:
  void OnLoadAndParseResource(
      ParsingErrorOr<ml::pipeline::TextProcessing> result);

  ml::pipeline::TextProcessing text_processing_pipeline_;

  base::WeakPtrFactory<TextClassification> weak_factory_{this};
};

}  // namespace brave_ads::resource

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
