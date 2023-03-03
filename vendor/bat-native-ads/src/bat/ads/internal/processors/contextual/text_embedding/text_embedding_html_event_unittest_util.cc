/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_event_unittest_util.h"

#include "base/base64.h"
#include "bat/ads/internal/common/crypto/crypto_util.h"
#include "bat/ads/internal/common/unittest/unittest_constants.h"

namespace ads {

ml::pipeline::TextEmbeddingInfo BuildTextEmbedding() {
  ml::pipeline::TextEmbeddingInfo text_embedding;
  text_embedding.text = "The quick brown fox jumps over the lazy dog";
  text_embedding.hashed_text_base64 =
      base::Base64Encode(crypto::Sha256(text_embedding.text));
  text_embedding.locale = kDefaultLocale;
  text_embedding.embedding = ml::VectorData({0.0853, -0.1789, 0.4221});

  return text_embedding;
}

}  // namespace ads
