/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/media/router/media_router_feature.h"

#include "build/build_config.h"
#include "content/public/browser/browser_context.h"

#define MediaRouterEnabled MediaRouterEnabled_ChromiumImpl
#define GlobalMediaControlsCastStartStopEnabled \
  GlobalMediaControlsCastStartStopEnabled_ChromiumImpl
#include "src/chrome/browser/media/router/media_router_feature.cc"
#undef GlobalMediaControlsCastStartStopEnabled
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
  return pref->GetValue()->GetBool();
#endif
}

#if !BUILDFLAG(IS_ANDROID)
// This override forces GlobalMediaControlsCastStartStopEnabled to use our
// version of MediaRouterEnabled, rather than the original. The implementation
// of this function must be kept in sync with the original implementation.
bool GlobalMediaControlsCastStartStopEnabled(content::BrowserContext* context) {
  return base::FeatureList::IsEnabled(kGlobalMediaControlsCastStartStop) &&
         MediaRouterEnabled(context);
}
#endif

}  // namespace media_router
