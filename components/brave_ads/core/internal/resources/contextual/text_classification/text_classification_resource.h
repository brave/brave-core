/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"
#include "brave/components/brave_ads/core/internal/resources/resource_parsing_error_or.h"

namespace brave_ads {

class TextClassificationResource final : public AdsClientNotifierObserver {
 public:
  TextClassificationResource();

  TextClassificationResource(const TextClassificationResource&) = delete;
  TextClassificationResource& operator=(const TextClassificationResource&) =
      delete;

  TextClassificationResource(TextClassificationResource&&) noexcept = delete;
  TextClassificationResource& operator=(TextClassificationResource&&) noexcept =
      delete;

  ~TextClassificationResource() override;

  bool IsInitialized() const;

  void Load();

  const ml::pipeline::TextProcessing& get() const {
    return text_processing_pipeline_;
  }

 private:
  void LoadAndParseResourceCallback(
      ResourceParsingErrorOr<ml::pipeline::TextProcessing> result);

  // AdsClientNotifierObserver:
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyDidUpdateResourceComponent(const std::string& id) override;

  ml::pipeline::TextProcessing text_processing_pipeline_;

  base::WeakPtrFactory<TextClassificationResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
