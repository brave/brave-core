/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/media/router/media_router_feature.h"

#include "content/public/browser/browser_context.h"

#define MediaRouterEnabled MediaRouterEnabled_ChromiumImpl
#include "../../../../../../chrome/browser/media/router/media_router_feature.cc"
#undef MediaRouterEnabled

namespace media_router {

bool MediaRouterEnabled(content::BrowserContext* context) {
  if (GetMediaRouterPref(context)->IsDefaultValue()) {
    return false;
  }
  return MediaRouterEnabled_ChromiumImpl(context);
}

}  // namespace media_router
