/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_RESOURCE_TEXT_EMBEDDING_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_RESOURCE_TEXT_EMBEDDING_RESOURCE_H_

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_parsing_error_or.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_processing.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"

namespace brave_ads {

class TextEmbeddingResource final : public AdsClientNotifierObserver {
 public:
  TextEmbeddingResource();

  TextEmbeddingResource(const TextEmbeddingResource&) = delete;
  TextEmbeddingResource& operator=(const TextEmbeddingResource&) = delete;

  TextEmbeddingResource(TextEmbeddingResource&&) noexcept = delete;
  TextEmbeddingResource& operator=(TextEmbeddingResource&&) noexcept = delete;

  ~TextEmbeddingResource() override;

  bool IsInitialized() const { return !!embedding_processing_; }

  const std::optional<ml::pipeline::EmbeddingProcessing>& get() const {
    return embedding_processing_;
  }

 private:
  void MaybeLoad();
  void MaybeLoadOrReset();

  bool DidLoad() const { return did_load_; }
  void Load();
  void LoadCallback(
      ResourceParsingErrorOr<ml::pipeline::EmbeddingProcessing> result);

  void MaybeReset();
  void Reset();

  // AdsClientNotifierObserver:
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyPrefDidChange(const std::string& path) override;
  void OnNotifyDidUpdateResourceComponent(const std::string& manifest_version,
                                          const std::string& id) override;
  void OnNotifyDidUnregisterResourceComponent(const std::string& id) override;

  std::optional<ml::pipeline::EmbeddingProcessing> embedding_processing_;

  bool did_load_ = false;
  std::optional<std::string> manifest_version_;

  base::WeakPtrFactory<TextEmbeddingResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_RESOURCE_TEXT_EMBEDDING_RESOURCE_H_
