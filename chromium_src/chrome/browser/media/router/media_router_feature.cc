/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/media/router/media_router_feature.h"

#include "chrome/common/pref_names.h"
#include "extensions/common/feature_switch.h"

#define MediaRouterEnabled MediaRouterEnabled_ChromiumImpl
#include "../../../../../../chrome/browser/media/router/media_router_feature.cc"  // NOLINT
#undef MediaRouterEnabled

namespace media_router {

// Media router pref can be toggled using kLoadMediaRouterComponentExtension
// on Desktop
void UpdateMediaRouterPref(content::BrowserContext* context) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  user_prefs::UserPrefs::Get(context)->SetBoolean(
      ::prefs::kEnableMediaRouter,
      extensions::FeatureSwitch::load_media_router_component_extension()
          ->IsEnabled());
#endif
}

bool MediaRouterEnabled(content::BrowserContext* context) {
#if defined(OS_ANDROID)
  return MediaRouterEnabled_ChromiumImpl(context);
#elif BUILDFLAG(ENABLE_EXTENSIONS)
  UpdateMediaRouterPref(context);
  const PrefService::Preference* pref = GetMediaRouterPref(context);
  bool allowed = false;
  CHECK(pref->GetValue()->GetAsBoolean(&allowed));

  // The component extension cannot be loaded in guest sessions.
  // crbug.com/756243
  return allowed && !Profile::FromBrowserContext(context)->IsGuestSession();
#else
  return false;
#endif  // defined(OS_ANDROID)
}

}  // namespace media_router
