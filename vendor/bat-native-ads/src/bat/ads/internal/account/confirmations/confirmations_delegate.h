/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_DELEGATE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_DELEGATE_H_

namespace ads {

struct ConfirmationInfo;

class ConfirmationsDelegate {
 public:
  // Invoked to tell the delegate that the |confirmation| was successfully sent.
  virtual void OnDidConfirm(const ConfirmationInfo& confirmation) {}

  // Invoked to tell the delegate that we failed to send the |confirmation|.
  virtual void OnFailedToConfirm(const ConfirmationInfo& confirmation) {}

 protected:
  virtual ~ConfirmationsDelegate() = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_DELEGATE_H_
