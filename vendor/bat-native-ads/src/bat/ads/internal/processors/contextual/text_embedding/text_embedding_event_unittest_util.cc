/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_event_unittest_util.h"

#include <string>
#include <vector>

#include "base/base64.h"
#include "base/guid.h"
#include "bat/ads/internal/base/crypto/crypto_util.h"
#include "bat/ads/internal/base/unittest/unittest_constants.h"
#include "bat/ads/internal/base/unittest/unittest_time_util.h"

namespace ads {

ml::pipeline::TextEmbeddingInfo BuildTextEmbedding() {
  const std::string text = base::GUID::GenerateRandomV4().AsLowercaseString();
  const std::vector<uint8_t> sha256_hash = security::Sha256(text);

  ml::pipeline::TextEmbeddingInfo text_embedding;
  text_embedding.text = text;
  text_embedding.text_hashed = base::Base64Encode(sha256_hash);
  text_embedding.embedding = ml::VectorData({0.0853, -0.1789, 0.4221});

  return text_embedding;
}

TextEmbeddingEventInfo BuildTextEmbeddingEvent() {
  ml::pipeline::TextEmbeddingInfo text_embedding = BuildTextEmbedding();
  TextEmbeddingEventInfo text_embedding_event;
  text_embedding_event.created_at = Now();
  text_embedding_event.version = "0";
  text_embedding_event.locale = kDefaultLocale;
  text_embedding_event.hashed_key = text_embedding.text_hashed;
  text_embedding_event.embedding = text_embedding.embedding.GetVectorAsString();

  return text_embedding_event;
}

}  // namespace ads
