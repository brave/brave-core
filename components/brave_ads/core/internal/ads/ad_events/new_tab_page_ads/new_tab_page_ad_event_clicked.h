/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_CLICKED_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_CLICKED_H_

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_interface.h"

namespace brave_ads {

struct NewTabPageAdInfo;

class NewTabPageAdEventClicked final
    : public AdEventInterface<NewTabPageAdInfo> {
 public:
  void FireEvent(const NewTabPageAdInfo& ad) override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_CLICKED_H_
