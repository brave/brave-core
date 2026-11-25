/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_SHARED_PINNED_TAB_EXCLUSION_MARKER_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_SHARED_PINNED_TAB_EXCLUSION_MARKER_H_

#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}  // namespace content

// Attached to a pinned tab's WebContents that must never be tracked or
// mirrored by SharedPinnedTabService, e.g. the sidebar web panel's tab.
//
// Attach this *before* the WebContents is inserted into any TabStripModel.
// Insertion notifies TabStripModelObservers (such as SharedPinnedTabService)
// synchronously, so attaching it only afterwards is already too late.
class SharedPinnedTabExclusionMarker
    : public content::WebContentsUserData<SharedPinnedTabExclusionMarker> {
 public:
  SharedPinnedTabExclusionMarker(const SharedPinnedTabExclusionMarker&) =
      delete;
  SharedPinnedTabExclusionMarker& operator=(
      const SharedPinnedTabExclusionMarker&) = delete;
  ~SharedPinnedTabExclusionMarker() override;

 private:
  explicit SharedPinnedTabExclusionMarker(content::WebContents* contents);

  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_SHARED_PINNED_TAB_EXCLUSION_MARKER_H_
