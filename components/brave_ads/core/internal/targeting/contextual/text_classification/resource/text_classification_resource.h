/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_RESOURCE_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_parsing_error_or.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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

  bool IsInitialized() const {
    return static_cast<bool>(text_processing_pipeline_);
  }

  const absl::optional<ml::pipeline::TextProcessing>& get() const {
    return text_processing_pipeline_;
  }

 private:
  void MaybeLoad();
  void MaybeLoadOrReset();

  bool DidLoad() const { return did_load_; }
  void Load();
  void LoadCallback(
      ResourceParsingErrorOr<ml::pipeline::TextProcessing> result);

  void MaybeReset();
  void Reset();

  // AdsClientNotifierObserver:
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyPrefDidChange(const std::string& path) override;
  void OnNotifyDidUpdateResourceComponent(const std::string& manifest_version,
                                          const std::string& id) override;

  absl::optional<ml::pipeline::TextProcessing> text_processing_pipeline_;

  bool did_load_ = false;
  absl::optional<std::string> manifest_version_;

  base::WeakPtrFactory<TextClassificationResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_RESOURCE_H_
