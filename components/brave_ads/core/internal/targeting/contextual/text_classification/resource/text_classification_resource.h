/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_RESOURCE_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/sequence_bound.h"
#include "base/types/expected.h"
#include "base/types/optional_ref.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

namespace brave_ads {

using ClassifyPageCallback =
    base::OnceCallback<void(base::optional_ref<const ml::PredictionMap>)>;

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

  bool IsLoaded() const { return !!text_processing_pipeline_; }

  std::optional<std::string> GetManifestVersion() const {
    return manifest_version_;
  }

  void ClassifyPage(const std::string& text, ClassifyPageCallback callback);

 private:
  void MaybeLoad();
  void MaybeLoadOrUnload();

  void Load();
  void LoadResourceComponentCallback(base::File file);
  void LoadCallback(base::expected<bool, std::string> result);

  void MaybeUnload();
  void Unload();

  // AdsClientNotifierObserver:
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyPrefDidChange(const std::string& path) override;
  void OnNotifyResourceComponentDidChange(const std::string& manifest_version,
                                          const std::string& id) override;
  void OnNotifyDidUnregisterResourceComponent(const std::string& id) override;

  std::optional<std::string> manifest_version_;

  std::optional<const base::SequenceBound<ml::pipeline::TextProcessing>>
      text_processing_pipeline_;

  base::WeakPtrFactory<TextClassificationResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_RESOURCE_H_
