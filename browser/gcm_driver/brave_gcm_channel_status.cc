/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/gcm_driver/brave_gcm_channel_status.h"

#include <memory>

#include "chrome/browser/profiles/profile.h"
#include "components/gcm_driver/gcm_channel_status_syncer.h"
#include "components/prefs/pref_service.h"

namespace gcm {

const char kBraveGCMStatusKey[] = "brave_gcm_channel_status";

// static
BraveGCMChannelStatus* BraveGCMChannelStatus::GetForProfile(
    Profile* profile) {
  BraveGCMChannelStatus* status = static_cast<BraveGCMChannelStatus*>(
      profile->GetUserData(kBraveGCMStatusKey));

  if (!status) {
    bool enabled = profile->GetPrefs()->GetBoolean(
        gcm::prefs::kGCMChannelStatus);
    // Object cleanup is handled by SupportsUserData
    profile->SetUserData(kBraveGCMStatusKey,
                        std::make_unique<BraveGCMChannelStatus>(enabled));
    status = static_cast<BraveGCMChannelStatus*>(
        profile->GetUserData(kBraveGCMStatusKey));
  }
  return status;
}

bool BraveGCMChannelStatus::IsGCMEnabled() {
  return gcm_enabled_;
}

}  // namespace gcm
