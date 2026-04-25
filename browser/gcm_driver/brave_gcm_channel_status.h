/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_GCM_DRIVER_BRAVE_GCM_CHANNEL_STATUS_H_
#define BRAVE_BROWSER_GCM_DRIVER_BRAVE_GCM_CHANNEL_STATUS_H_

#include "base/memory/raw_ptr.h"
#include "base/supports_user_data.h"

class Profile;

namespace gcm {

class BraveGCMChannelStatus : public base::SupportsUserData::Data {
 public:
  explicit BraveGCMChannelStatus(Profile* profile, bool enabled);
  static BraveGCMChannelStatus* GetForProfile(Profile *profile);

  bool IsGCMEnabled() const;
  void UpdateGCMDriverStatus();

 private:
  raw_ptr<Profile> profile_ = nullptr;
  bool gcm_enabled_;
};

}  // namespace gcm

#endif  // BRAVE_BROWSER_GCM_DRIVER_BRAVE_GCM_CHANNEL_STATUS_H_
