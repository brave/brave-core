/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_RESOURCE_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_info.h"
#include "brave/components/brave_ads/core/internal/resources/async/resource_async_handler.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/embedding_processing_ref_counted_proxy.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

using EmbedTextCallback =
    base::OnceCallback<void(const ml::pipeline::TextEmbeddingInfo&)>;

class TextEmbeddingResource final : public AdsClientNotifierObserver {
 public:
  TextEmbeddingResource();

  TextEmbeddingResource(const TextEmbeddingResource&) = delete;
  TextEmbeddingResource& operator=(const TextEmbeddingResource&) = delete;

  TextEmbeddingResource(TextEmbeddingResource&&) noexcept = delete;
  TextEmbeddingResource& operator=(TextEmbeddingResource&&) noexcept = delete;

  ~TextEmbeddingResource() override;

  bool IsInitialized() const {
    return static_cast<bool>(embedding_processing_);
  }

  void EmbedText(const std::string& text, EmbedTextCallback callback) const;

 private:
  void MaybeLoad();
  void MaybeLoadOrReset();

  bool DidLoad() const { return did_load_; }
  void Load();
  void OnLoadFileResource(base::File file);
  void LoadCallback(base::expected<bool, std::string> result);

  void MaybeReset();
  void Reset();

  // AdsClientNotifierObserver:
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyPrefDidChange(const std::string& path) override;
  void OnNotifyDidUpdateResourceComponent(const std::string& manifest_version,
                                          const std::string& id) override;

  absl::optional<ResourceAsyncHandler<EmbeddingProcessingRefCountedProxy>>
      embedding_processing_;

  bool did_load_ = false;
  absl::optional<std::string> manifest_version_;

  base::WeakPtrFactory<TextEmbeddingResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_RESOURCE_H_
