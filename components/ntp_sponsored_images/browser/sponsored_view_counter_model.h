// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_SPONSORED_VIEW_COUNTER_MODEL_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_SPONSORED_VIEW_COUNTER_MODEL_H_

#include "brave/components/ntp_sponsored_images/browser/view_counter_model.h"

namespace ntp_sponsored_images {

class SponsoredViewCounterModel : public ViewCounterModel {
 public:
  SponsoredViewCounterModel();
  ~SponsoredViewCounterModel() override;

  SponsoredViewCounterModel(const SponsoredViewCounterModel&) = delete;
  SponsoredViewCounterModel& operator=(
      const SponsoredViewCounterModel&) = delete;

  // ViewCounterModel overrides:
  bool ShouldShowWallpaper() const override;
  void RegisterPageView() override;

 private:
  int count_to_branded_wallpaper_;
};

}  // namespace ntp_sponsored_images


#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_SPONSORED_VIEW_COUNTER_MODEL_H_
