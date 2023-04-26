/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/ui/views/global_media_controls/media_item_ui_helper.h"

// Don't try to get session route when the feature is disabled. Upstream folks
// seem to miss this.
// https://github.com/brave/brave-browser/issues/29999
#define GetSessionRoute(ID, ITEM, PROFILE)  \
  media_router::MediaRouterEnabled(PROFILE) \
      ? GetSessionRoute(ID, ITEM, PROFILE)  \
      : absl::nullopt;

#include "src/chrome/browser/ui/views/global_media_controls/media_dialog_view.cc"

#undef GetSessionRoute
