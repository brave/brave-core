/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_event_unittest_util.h"

#include <string>
#include <vector>

#include "base/base64.h"
#include "bat/ads/internal/base/crypto/crypto_util.h"
#include "bat/ads/internal/base/unittest/unittest_constants.h"
#include "bat/ads/internal/base/unittest/unittest_time_util.h"

namespace ads {

ml::pipeline::TextEmbeddingInfo BuildTextEmbedding() {
  const std::string text = "The quick brown fox jumps over the lazy dog";
  ml::pipeline::TextEmbeddingInfo text_embedding;
  text_embedding.text = text;
  text_embedding.hashed_text_base64 =
      base::Base64Encode(security::Sha256(text));
  text_embedding.embedding = ml::VectorData({0.0853, -0.1789, 0.4221});

  return text_embedding;
}

TextEmbeddingHtmlEventInfo BuildTextEmbeddingHtmlEvent() {
  ml::pipeline::TextEmbeddingInfo text_embedding = BuildTextEmbedding();
  TextEmbeddingHtmlEventInfo text_embedding_html_event;
  text_embedding_html_event.created_at = Now();
  text_embedding_html_event.version = "0";
  text_embedding_html_event.locale = kDefaultLocale;
  text_embedding_html_event.hashed_text_base64 =
      text_embedding.hashed_text_base64;
  text_embedding_html_event.embedding =
      text_embedding.embedding.GetVectorAsString();

  return text_embedding_html_event;
}

}  // namespace ads
