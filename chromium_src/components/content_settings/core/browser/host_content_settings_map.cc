/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "components/content_settings/core/browser/content_settings_utils.h"

#if !BUILDFLAG(IS_IOS)
#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"
#define PrefProvider BravePrefProvider
#endif

namespace content_settings {
namespace {

bool IsMorePermissive_BraveImpl(ContentSettingsType content_type,
                                ContentSetting a,
                                ContentSetting b) {
  // NOTIFICATIONS is auto-blocked in incognito after a random timeout, it
  // requires special handling.
  if (content_type == ContentSettingsType::NOTIFICATIONS) {
    return IsMorePermissive(a, b);
  }
  return true;
}

}  // namespace
}  // namespace content_settings

#define IsMorePermissive(a, b) IsMorePermissive_BraveImpl(content_type, a, b)

#include "src/components/content_settings/core/browser/host_content_settings_map.cc"

#undef IsMorePermissive

#if !BUILDFLAG(IS_IOS)
#undef PrefProvider
#endif
