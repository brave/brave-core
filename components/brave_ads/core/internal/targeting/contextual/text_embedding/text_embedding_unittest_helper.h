/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_UNITTEST_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_UNITTEST_HELPER_H_

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/resource/text_embedding_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_processor.h"

namespace brave_ads::test {

class TextEmbeddingHelper final {
 public:
  TextEmbeddingHelper();

  TextEmbeddingHelper(const TextEmbeddingHelper&) = delete;
  TextEmbeddingHelper& operator=(const TextEmbeddingHelper&) = delete;

  TextEmbeddingHelper(TextEmbeddingHelper&&) noexcept = delete;
  TextEmbeddingHelper& operator=(TextEmbeddingHelper&&) noexcept = delete;

  ~TextEmbeddingHelper();

  void Mock();

  static TextEmbeddingHtmlEventList Expectation();

 private:
  TextEmbeddingResource resource_;
  TextEmbeddingProcessor processor_;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_UNITTEST_HELPER_H_
