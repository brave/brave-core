/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_HISTORY_AD_HISTORY_ITEM_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_HISTORY_AD_HISTORY_ITEM_INFO_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/public/export.h"
#include "url/gurl.h"

namespace brave_ads {

struct ADS_EXPORT AdHistoryItemInfo final {
  AdHistoryItemInfo();

  AdHistoryItemInfo(const AdHistoryItemInfo&);
  AdHistoryItemInfo& operator=(const AdHistoryItemInfo&);

  AdHistoryItemInfo(AdHistoryItemInfo&&) noexcept;
  AdHistoryItemInfo& operator=(AdHistoryItemInfo&&) noexcept;

  ~AdHistoryItemInfo();

  bool operator==(const AdHistoryItemInfo&) const = default;

  [[nodiscard]] bool IsValid() const;

  base::Time created_at;
  AdType type = AdType::kUndefined;
  ConfirmationType confirmation_type = ConfirmationType::kUndefined;
  std::string placement_id;
  std::string creative_instance_id;
  std::string creative_set_id;
  std::string campaign_id;
  std::string advertiser_id;
  std::string segment;
  std::string title;
  std::string description;
  GURL target_url;
};

using AdHistoryList = std::vector<AdHistoryItemInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_HISTORY_AD_HISTORY_ITEM_INFO_H_
