/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_OBSERVER_H_

#include <cstdint>

#include "base/observer_list_types.h"
#include "base/time/time.h"

namespace brave_ads {

struct AdInfo;
struct TabInfo;

class SiteVisitObserver : public base::CheckedObserver {
 public:
  // Invoked when there is a possibility that the user will arrive at the
  // landing page for the given `ad` `after` a period of time.
  virtual void OnMaybeLandOnPage(const AdInfo& ad,
                                 const base::TimeDelta after) {}

  // Invoked when the given `tab` for a page becomes occluded and will resume
  // after `remaining_time` when the tab becomes visible.
  virtual void OnDidSuspendPageLand(const TabInfo& tab,
                                    const base::TimeDelta remaining_time) {}

  // Invoked when the given `tab` for a page becomes visible and may load after
  // 'remaining_time'.
  virtual void OnDidResumePageLand(const TabInfo& tab,
                                   const base::TimeDelta remaining_time) {}

  // Invoked when a user landed on the landing page associated with the given
  // `tab` and `ad`.
  virtual void OnDidLandOnPage(const TabInfo& tab, const AdInfo& ad) {}

  // Invoked when the user did not land on the landing page for the given `tab`
  // and `ad`.
  virtual void OnDidNotLandOnPage(const TabInfo& tab, const AdInfo& ad) {}

  // Invoked when canceling a page land for the given `tab_id` and `ad`.
  virtual void OnCanceledPageLand(const int32_t tab_id, const AdInfo& ad) {}
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_OBSERVER_H_
