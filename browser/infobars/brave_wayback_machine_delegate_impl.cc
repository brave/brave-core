/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_wayback_machine_delegate_impl.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

BraveWaybackMachineDelegateImpl::BraveWaybackMachineDelegateImpl(
    Profile* profile)
    : profile_(profile) {
}

BraveWaybackMachineDelegateImpl::~BraveWaybackMachineDelegateImpl() = default;

bool BraveWaybackMachineDelegateImpl::IsWaybackMachineEnabled() const {
  return profile_->GetPrefs()->GetBoolean(kBraveWaybackMachineEnabled);
}
