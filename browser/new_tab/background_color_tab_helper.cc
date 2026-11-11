/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/background_color_tab_helper.h"

#include "chrome/browser/ui/color/chrome_color_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/color/color_provider.h"

BackgroundColorTabHelper::BackgroundColorTabHelper(tabs::TabInterface& tab)
    : tabs::ContentsObservingTabFeature(tab) {}

BackgroundColorTabHelper::~BackgroundColorTabHelper() = default;

void BackgroundColorTabHelper::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host->GetParent()) {
    return;
  }

  auto* view = render_frame_host->GetView();
  if (view) {
    view->SetBackgroundColor(web_contents()->GetColorProvider().GetColor(
        kColorNewTabPageBackground));
  }
}
