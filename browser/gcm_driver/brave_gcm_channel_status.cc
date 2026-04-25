/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/gcm_driver/brave_gcm_channel_status.h"

#include <memory>

#include "brave/components/constants/pref_names.h"
#include "chrome/browser/gcm/gcm_profile_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/gcm_driver/gcm_driver_desktop.h"
#include "components/gcm_driver/gcm_profile_service.h"
#include "components/prefs/pref_service.h"

namespace gcm {

constexpr char kBraveGCMStatusKey[] = "brave_gcm_channel_status";

BraveGCMChannelStatus::BraveGCMChannelStatus(Profile* profile, bool enabled)
    : profile_(profile), gcm_enabled_(enabled) {}

// static
BraveGCMChannelStatus* BraveGCMChannelStatus::GetForProfile(
    Profile* profile) {
  BraveGCMChannelStatus* status = static_cast<BraveGCMChannelStatus*>(
      profile->GetUserData(kBraveGCMStatusKey));

  if (!status) {
    bool enabled = profile->GetPrefs()->GetBoolean(kBraveGCMChannelStatus);
    // Object cleanup is handled by SupportsUserData
    profile->SetUserData(
        kBraveGCMStatusKey,
        std::make_unique<BraveGCMChannelStatus>(profile, enabled));
    status = static_cast<BraveGCMChannelStatus*>(
        profile->GetUserData(kBraveGCMStatusKey));
  }
  return status;
}

bool BraveGCMChannelStatus::IsGCMEnabled() const {
  return gcm_enabled_;
}

void BraveGCMChannelStatus::UpdateGCMDriverStatus() {
  if (!profile_)
    return;
  gcm::GCMProfileService* gcm_profile_service =
      gcm::GCMProfileServiceFactory::GetForProfile(profile_);
  if (!gcm_profile_service)
    return;
  gcm::GCMDriver* gcm_driver = gcm_profile_service->driver();
  if (!gcm_driver)
    return;
  gcm_driver->SetEnabled(IsGCMEnabled());
}

}  // namespace gcm
