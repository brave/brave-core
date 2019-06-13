/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_process_manager_delegate.h"

#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/process_manager.h"
#include "extensions/browser/process_manager_factory.h"

namespace extensions {

BraveProcessManagerDelegate::BraveProcessManagerDelegate() {
}

BraveProcessManagerDelegate::~BraveProcessManagerDelegate() {
}

void BraveProcessManagerDelegate::OnProfileDestroyed(Profile* profile) {
  ChromeProcessManagerDelegate::OnProfileDestroyed(profile);

  // If this profile owns an incognito profile, but it is destroyed before the
  // tor profile is destroyed, then close the background hosts in tor profile
  // as well.
  if (!profile->IsTorProfile() && profile->HasTorProfile()) {
    extensions::ProcessManager* tor_manager =
      extensions::ProcessManagerFactory::GetForBrowserContextIfExists(
          profile->GetTorProfile());
    if (tor_manager) {
      tor_manager->CloseBackgroundHosts();
    }
  }
}

}  // namespace extensions
