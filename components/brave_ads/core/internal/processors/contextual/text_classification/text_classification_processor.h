/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"

class GURL;

namespace brave_ads {

namespace resource {
class TextClassification;
}  // namespace resource

namespace processor {

class TextClassification final : public AdsClientNotifierObserver,
                                 public TabManagerObserver {
 public:
  explicit TextClassification(resource::TextClassification& resource);

  TextClassification(const TextClassification&) = delete;
  TextClassification& operator=(const TextClassification&) = delete;

  TextClassification(TextClassification&&) noexcept = delete;
  TextClassification& operator=(TextClassification&&) noexcept = delete;

  ~TextClassification() override;

  void Process(const std::string& text);

 private:
  // AdsClientNotifierObserver:
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyDidUpdateResourceComponent(const std::string& id) override;

  // TabManagerObserver:
  void OnTextContentDidChange(int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              const std::string& content) override;

  const raw_ref<resource::TextClassification> resource_;
};

}  // namespace processor
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_
