/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/resources/parsing_result.h"

namespace brave_ads {

namespace ml::pipeline {
class TextProcessing;
}  // namespace ml::pipeline

namespace resource {

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

  ml::pipeline::TextProcessing* Get() const;

 private:
  void OnLoadAndParseResource(
      ParsingResultPtr<ml::pipeline::TextProcessing> result);

  std::unique_ptr<ml::pipeline::TextProcessing> text_processing_pipeline_;

  base::WeakPtrFactory<TextClassification> weak_factory_{this};
};

}  // namespace resource
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
