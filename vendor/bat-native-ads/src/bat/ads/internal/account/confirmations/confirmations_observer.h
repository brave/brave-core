/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_OBSERVER_H_

#include "base/observer_list_types.h"

namespace ads {

struct ConfirmationInfo;

class ConfirmationsObserver : public base::CheckedObserver {
 public:
  // Invoked when a confirmation was sent
  virtual void OnDidConfirm(const double estimated_redemption_value,
                            const ConfirmationInfo& confirmation) {}

  // Invoked when a confirmation failed to send
  virtual void OnFailedToConfirm(const ConfirmationInfo& confirmation) {}

 protected:
  ~ConfirmationsObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_OBSERVER_H_
