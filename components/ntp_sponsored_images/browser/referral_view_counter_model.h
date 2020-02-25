// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_REFERRAL_VIEW_COUNTER_MODEL_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_REFERRAL_VIEW_COUNTER_MODEL_H_

#include "brave/components/ntp_sponsored_images/browser/view_counter_model.h"

namespace ntp_sponsored_images {

class ReferralViewCounterModel : public ViewCounterModel {
 public:
  ReferralViewCounterModel();
  ~ReferralViewCounterModel() override;

  ReferralViewCounterModel(const ReferralViewCounterModel&) = delete;
  ReferralViewCounterModel operator=(const ReferralViewCounterModel&) = delete;

  // Show referral wallpaper in NTP always.
  bool ShouldShowWallpaper() const override;
  void RegisterPageView() override;
};

}  // namespace ntp_sponsored_images

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_REFERRAL_VIEW_COUNTER_MODEL_H_

