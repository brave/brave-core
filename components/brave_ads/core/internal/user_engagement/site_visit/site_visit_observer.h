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

class SiteVisitObserver : public base::CheckedObserver {
 public:
  // Invoked when there is a possibility that the user will arrive at the
  // landing page for the given `ad` `after` a period of time.
  virtual void OnMaybeLandOnPage(const AdInfo& ad, base::TimeDelta after) {}

  // Invoked when the given `tab_id` for a page becomes occluded and will resume
  // after `remaining_time` when the tab becomes visible.
  virtual void OnDidSuspendPageLand(int32_t tab_id,
                                    base::TimeDelta remaining_time) {}

  // Invoked when the given `tab_id` for a page becomes visible and may load
  // after 'remaining_time'.
  virtual void OnDidResumePageLand(int32_t tab_id,
                                   base::TimeDelta remaining_time) {}

  // Invoked when a user landed on the landing page associated with the given
  // `tab_id`, `http_status_code`, and `ad`.
  virtual void OnDidLandOnPage(int32_t tab_id,
                               int http_status_code,
                               const AdInfo& ad) {}

  // Invoked when the user did not land on the landing page for the given
  // `tab_id` and `ad`.
  virtual void OnDidNotLandOnPage(int32_t tab_id, const AdInfo& ad) {}

  // Invoked when canceling a page land for the given `tab_id` and `ad`.
  virtual void OnCanceledPageLand(int32_t tab_id, const AdInfo& ad) {}
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_OBSERVER_H_
