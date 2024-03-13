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
  // Invoked if the user may land on the landing page for the given `ad` at
  // `maybe_at` time.
  virtual void OnMaybeLandOnPage(const AdInfo& ad, const base::Time maybe_at) {}

  // Invoked when the user landed on the landing page for the given `tab` and
  // `ad`.
  virtual void OnDidLandOnPage(const TabInfo& tab, const AdInfo& ad) {}

  // Invoked when the user did not land on the landing page for the given `ad`.
  virtual void OnDidNotLandOnPage(const AdInfo& ad) {}

  // Invoked when canceling a page land for the given `ad` and `tab_id`.
  virtual void OnCanceledPageLand(const AdInfo& ad, const int32_t tab_id) {}
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_OBSERVER_H_
