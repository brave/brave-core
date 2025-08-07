/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ssl/https_upgrades_util.h"

#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_handle.h"

namespace {

bool NormalWindowHttpsOnly(const GURL& url,
                           Profile* profile) {
  if (profile->IsIncognitoProfile()) {
    return false;
  }
  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  return brave_shields::ShouldForceHttps(map, url);
}

}  // namespace

#include <chrome/browser/ssl/https_upgrades_util.cc>
