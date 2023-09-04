/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"

class GURL;

namespace brave_ads {

class TextClassificationResource;

class TextClassificationProcessor final : public TabManagerObserver {
 public:
  explicit TextClassificationProcessor(TextClassificationResource& resource);

  TextClassificationProcessor(const TextClassificationProcessor&) = delete;
  TextClassificationProcessor& operator=(const TextClassificationProcessor&) =
      delete;

  TextClassificationProcessor(TextClassificationProcessor&&) noexcept = delete;
  TextClassificationProcessor& operator=(
      TextClassificationProcessor&&) noexcept = delete;

  ~TextClassificationProcessor() override;

  void Process(const std::string& text);

 private:
  // TabManagerObserver:
  void OnTextContentDidChange(int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              const std::string& text) override;

  const raw_ref<TextClassificationResource> resource_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_PROCESSOR_H_
