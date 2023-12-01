/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_unittest_util.h"

#include "base/base64.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_info.h"

namespace brave_ads::ml::pipeline::test {

ml::pipeline::TextEmbeddingInfo BuildTextEmbedding() {
  TextEmbeddingInfo text_embedding;

  text_embedding.text = "The quick brown fox jumps over the lazy dog";
  text_embedding.hashed_text_base64 =
      base::Base64Encode(crypto::Sha256(text_embedding.text));
  text_embedding.locale = kDefaultLocale;
  text_embedding.embedding = {0.0853, -0.1789, 0.4221};

  return text_embedding;
}

}  // namespace brave_ads::ml::pipeline::test
