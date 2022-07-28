/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_event_unittest_util.h"

#include "base/guid.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_event_info.h"

namespace ads {

TextEmbeddingEventInfo BuildTextEmbeddingEvent() {
  TextEmbeddingEventInfo text_embedding_event;
  text_embedding_event.created_at = base::Time::Now();
  text_embedding_event.version = "";
  text_embedding_event.locale = "";
  text_embedding_event.hashed_key = base::GUID::GenerateRandomV4().AsLowercaseString();
  text_embedding_event.embedding = "0.0853 -0.1789 0.4221";

  return text_embedding_event;
}

}  // namespace ads