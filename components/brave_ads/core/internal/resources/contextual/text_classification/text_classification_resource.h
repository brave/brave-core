/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_alias.h"
#include "brave/components/brave_ads/core/internal/resources/async/resource_async_handler.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_processing_ref_counted_proxy.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

using ClassifyPageCallback =
    base::OnceCallback<void(const TextClassificationProbabilityMap&)>;

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

  void ClassifyPage(const std::string& text, ClassifyPageCallback callback);

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

  absl::optional<ResourceAsyncHandler<TextProcessingRefCountedProxy>>
      text_processing_pipeline_;

  bool did_load_ = false;
  absl::optional<std::string> manifest_version_;

  base::WeakPtrFactory<TextClassificationResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_RESOURCE_H_
