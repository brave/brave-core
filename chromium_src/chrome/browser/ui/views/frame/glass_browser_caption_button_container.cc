/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/glass_browser_caption_button_container.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/ui/views/tab_search_bubble_host.h"

#define GlassBrowserCaptionButtonContainer \
  GlassBrowserCaptionButtonContainer_ChromiumImpl

#include "src/chrome/browser/ui/views/frame/glass_browser_caption_button_container.cc"
#undef GlassBrowserCaptionButtonContainer

GlassBrowserCaptionButtonContainer::GlassBrowserCaptionButtonContainer(
    GlassBrowserFrameView* frame_view)
    : GlassBrowserCaptionButtonContainer_ChromiumImpl(frame_view),
      frame_view_(frame_view) {
  if (WindowFrameUtil::IsWin10TabSearchCaptionButtonEnabled(
          frame_view_->browser_view()->browser())) {
    pref_change_registrar_.Init(
        frame_view_->browser_view()->GetProfile()->GetPrefs());
    pref_change_registrar_.Add(
        kTabsSearchShow,
        base::BindRepeating(
            &GlassBrowserCaptionButtonContainer::OnPreferenceChanged,
            base::Unretained(this)));
    // Show the correct value in settings on initial start
    UpdateSearchTabsButtonState();
  }
}

GlassBrowserCaptionButtonContainer::~GlassBrowserCaptionButtonContainer() =
    default;

void GlassBrowserCaptionButtonContainer::OnPreferenceChanged(
    const std::string& pref_name) {
  if (pref_name == kTabsSearchShow) {
    UpdateSearchTabsButtonState();
    return;
  }
}

void GlassBrowserCaptionButtonContainer::UpdateSearchTabsButtonState() {
  auto* tab_search_bubble_host = GetTabSearchBubbleHost();
  if (tab_search_bubble_host) {
    auto* button = tab_search_bubble_host->button();
    bool is_tab_search_visible =
        frame_view_->browser_view()->GetProfile()->GetPrefs()->GetBoolean(
            kTabsSearchShow);
    button->SetVisible(is_tab_search_visible);
  }
}
