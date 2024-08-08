/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_REACTIONS_REACTIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_REACTIONS_REACTIONS_H_

#include "brave/components/brave_ads/core/internal/history/ad_history_manager_observer.h"

namespace brave_ads {

struct AdHistoryItemInfo;

class Reactions final : public AdHistoryManagerObserver {
 public:
  Reactions();

  Reactions(const Reactions&) = delete;
  Reactions& operator=(const Reactions&) = delete;

  Reactions(Reactions&&) noexcept = delete;
  Reactions& operator=(Reactions&&) noexcept = delete;

  ~Reactions() override;

 private:
  // AdHistoryManagerObserver:
  void OnDidLikeAd(const AdHistoryItemInfo& ad_history_item) override;
  void OnDidDislikeAd(const AdHistoryItemInfo& ad_history_item) override;
  void OnDidMarkAdAsInappropriate(
      const AdHistoryItemInfo& ad_history_item) override;
  void OnDidSaveAd(const AdHistoryItemInfo& ad_history_item) override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_REACTIONS_REACTIONS_H_
