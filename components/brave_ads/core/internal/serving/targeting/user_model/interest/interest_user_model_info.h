/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_USER_MODEL_INTEREST_INTEREST_USER_MODEL_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_USER_MODEL_INTEREST_INTEREST_USER_MODEL_INFO_H_

#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_info.h"

namespace brave_ads {

struct InterestUserModelInfo final {
  InterestUserModelInfo();
  InterestUserModelInfo(SegmentList segments,
                        TextEmbeddingHtmlEventList text_embedding_html_events);

  InterestUserModelInfo(const InterestUserModelInfo&);
  InterestUserModelInfo& operator=(const InterestUserModelInfo&);

  InterestUserModelInfo(InterestUserModelInfo&&) noexcept;
  InterestUserModelInfo& operator=(InterestUserModelInfo&&) noexcept;

  ~InterestUserModelInfo();

  bool operator==(const InterestUserModelInfo&) const = default;

  SegmentList segments;
  TextEmbeddingHtmlEventList text_embedding_html_events;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_USER_MODEL_INTEREST_INTEREST_USER_MODEL_INFO_H_
