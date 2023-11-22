/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_unittest_helper.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/resource/text_embedding_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_processor.h"

namespace brave_ads::test {

TextEmbeddingHelper::TextEmbeddingHelper() : processor_(resource_) {}

TextEmbeddingHelper::~TextEmbeddingHelper() = default;

void TextEmbeddingHelper::Mock() {
  processor_.Process(
      /*html=*/
      R"(<meta property="og:title" content="This simple unittest mock checks for embedding accuracy." />)");
}

// static
TextEmbeddingHtmlEventList TextEmbeddingHelper::Expectation() {
  TextEmbeddingHtmlEventList expected_text_embedding_html_events;

  TextEmbeddingHtmlEventInfo expected_text_embedding_html_event;
  expected_text_embedding_html_event.created_at = Now();
  expected_text_embedding_html_event.locale = "EN";
  expected_text_embedding_html_event.hashed_text_base64 =
      "LgRmA8VmL0dmu9ka3k1OamEx1AkHptfGIXstbZCFfmY=";
  expected_text_embedding_html_event.embedding = {0.5, 0.4, 1.0};
  expected_text_embedding_html_events.push_back(
      expected_text_embedding_html_event);

  return expected_text_embedding_html_events;
}

}  // namespace brave_ads::test
