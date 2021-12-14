/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_GLASS_BROWSER_CAPTION_BUTTON_CONTAINER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_GLASS_BROWSER_CAPTION_BUTTON_CONTAINER_H_

#include <string>

#include "components/prefs/pref_change_registrar.h"

#define GlassBrowserCaptionButtonContainer \
  GlassBrowserCaptionButtonContainer_ChromiumImpl

#include "src/chrome/browser/ui/views/frame/glass_browser_caption_button_container.h"
#undef GlassBrowserCaptionButtonContainer

class GlassBrowserCaptionButtonContainer
    : public GlassBrowserCaptionButtonContainer_ChromiumImpl {
 public:
  explicit GlassBrowserCaptionButtonContainer(
      GlassBrowserFrameView* frame_view);
  ~GlassBrowserCaptionButtonContainer() override;

 private:
  void UpdateSearchTabsButtonState();
  void OnPreferenceChanged(const std::string& pref_name);

  const raw_ptr<GlassBrowserFrameView> frame_view_;
  PrefChangeRegistrar pref_change_registrar_;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_GLASS_BROWSER_CAPTION_BUTTON_CONTAINER_H_
