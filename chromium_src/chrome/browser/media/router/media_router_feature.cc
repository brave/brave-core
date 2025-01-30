/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/media/router/media_router_feature.h"

#include "build/build_config.h"
#include "content/public/browser/browser_context.h"

#define MediaRouterEnabled MediaRouterEnabled_ChromiumImpl
#include "src/chrome/browser/media/router/media_router_feature.cc"
#undef MediaRouterEnabled

namespace media_router {

bool MediaRouterEnabled(content::BrowserContext* context) {
#if BUILDFLAG(IS_ANDROID)
  return MediaRouterEnabled_ChromiumImpl(context);
#else
  if (!base::FeatureList::IsEnabled(kMediaRouter)) {
    return false;
  }
  const PrefService::Preference* pref = GetMediaRouterPref(context);
  CHECK(pref->GetValue()->is_bool());
  // Chromium has a pref for Media Router but it is only controlled via
  // enterprise policy. In Brave, the pref can be controlled both via
  // brave://settings/extensions and enterprise policy, with the latter taking
  // precedence.
  if (pref->IsManaged()) {
    return MediaRouterEnabled_ChromiumImpl(context);
  }
  return pref->GetValue()->GetBool();
#endif
}

}  // namespace media_router
