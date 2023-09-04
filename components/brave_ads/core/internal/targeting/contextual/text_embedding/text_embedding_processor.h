/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_PROCESSOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_PROCESSOR_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"

class GURL;

namespace brave_ads {

class TextEmbeddingResource;

class TextEmbeddingProcessor final : public TabManagerObserver {
 public:
  explicit TextEmbeddingProcessor(TextEmbeddingResource& resource);

  TextEmbeddingProcessor(const TextEmbeddingProcessor&) = delete;
  TextEmbeddingProcessor& operator=(const TextEmbeddingProcessor&) = delete;

  TextEmbeddingProcessor(TextEmbeddingProcessor&&) noexcept = delete;
  TextEmbeddingProcessor& operator=(TextEmbeddingProcessor&&) noexcept = delete;

  ~TextEmbeddingProcessor() override;

  void Process(const std::string& html);

 private:
  // TabManagerObserver:
  void OnHtmlContentDidChange(int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              const std::string& html) override;

  const raw_ref<TextEmbeddingResource> resource_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_PROCESSOR_H_
