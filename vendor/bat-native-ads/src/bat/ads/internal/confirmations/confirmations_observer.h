/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CONFIRMATIONS_CONFIRMATIONS_OBSERVER_H_
#define BAT_ADS_INTERNAL_CONFIRMATIONS_CONFIRMATIONS_OBSERVER_H_

#include "base/observer_list.h"

namespace ads {

struct ConfirmationInfo;

class ConfirmationsObserver : public base::CheckedObserver {
 public:
  // Invoked when an ad confirmation is successful
  virtual void OnConfirmAd(
      const double estimated_redemption_value,
      const ConfirmationInfo& confirmation) {}

  // Invoked when an ad confirmation fails
  virtual void OnConfirmAdFailed(
      const ConfirmationInfo& confirmation) {}

 protected:
  ~ConfirmationsObserver() override = default;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CONFIRMATIONS_CONFIRMATIONS_OBSERVER_H_
