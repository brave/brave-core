/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/public/shared_pinned_tab_exclusion_marker.h"

#include "content/public/browser/web_contents.h"

SharedPinnedTabExclusionMarker::SharedPinnedTabExclusionMarker(
    content::WebContents* contents)
    : content::WebContentsUserData<SharedPinnedTabExclusionMarker>(*contents) {}

SharedPinnedTabExclusionMarker::~SharedPinnedTabExclusionMarker() = default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(SharedPinnedTabExclusionMarker);
