/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_USER_MODEL_LATENT_INTEREST_LATENT_INTEREST_USER_MODEL_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_USER_MODEL_LATENT_INTEREST_LATENT_INTEREST_USER_MODEL_INFO_H_

#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

struct LatentInterestUserModelInfo final {
  LatentInterestUserModelInfo();
  explicit LatentInterestUserModelInfo(SegmentList segments);

  LatentInterestUserModelInfo(const LatentInterestUserModelInfo&);
  LatentInterestUserModelInfo& operator=(const LatentInterestUserModelInfo&);

  LatentInterestUserModelInfo(LatentInterestUserModelInfo&&) noexcept;
  LatentInterestUserModelInfo& operator=(
      LatentInterestUserModelInfo&&) noexcept;

  ~LatentInterestUserModelInfo();

  bool operator==(const LatentInterestUserModelInfo&) const = default;

  SegmentList segments;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_USER_MODEL_LATENT_INTEREST_LATENT_INTEREST_USER_MODEL_INFO_H_
