/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/constants/pref_names.h"

#define BRAVE_MANAGED_CONTENT_SETTINGS                                         \
  {kManagedBraveShieldsDisabledForUrls, ContentSettingsType::BRAVE_SHIELDS,    \
   CONTENT_SETTING_BLOCK},                                                     \
      {kManagedBraveShieldsEnabledForUrls, ContentSettingsType::BRAVE_SHIELDS, \
       CONTENT_SETTING_ALLOW},

#define BRAVE_MANAGED_PREFS \
  kManagedBraveShieldsDisabledForUrls, kManagedBraveShieldsEnabledForUrls,

#include "src/components/content_settings/core/browser/content_settings_policy_provider.cc"
#undef BRAVE_MANAGED_PREFS
#undef BRAVE_MANAGED_CONTENT_SETTINGS
