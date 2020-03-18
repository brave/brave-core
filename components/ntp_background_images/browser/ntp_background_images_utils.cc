// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/ntp_background_images_utils.h"

#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace ntp_background_images {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kNewTabPageShowBackgroundImage, true);
}

}  // namespace ntp_background_images
