/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NEW_TAB_BACKGROUND_COLOR_TAB_HELPER_H_
#define BRAVE_BROWSER_NEW_TAB_BACKGROUND_COLOR_TAB_HELPER_H_

#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"

namespace tabs {
class TabInterface;
}  // namespace tabs

// Sets root RenderWidgetHostView background color to the ntp background color
// to avoid white flash on a new tab (Windows-specific).
class BackgroundColorTabHelper : public tabs::ContentsObservingTabFeature {
 public:
  explicit BackgroundColorTabHelper(tabs::TabInterface& tab);
  ~BackgroundColorTabHelper() override;

  BackgroundColorTabHelper(const BackgroundColorTabHelper&) = delete;
  BackgroundColorTabHelper& operator=(const BackgroundColorTabHelper&) = delete;

 private:
  // content::WebContentsObserver:
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
};

#endif  // BRAVE_BROWSER_NEW_TAB_BACKGROUND_COLOR_TAB_HELPER_H_
