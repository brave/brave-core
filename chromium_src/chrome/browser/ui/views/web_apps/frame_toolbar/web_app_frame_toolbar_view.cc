/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <chrome/browser/ui/views/web_apps/frame_toolbar/web_app_frame_toolbar_view.cc>

ToolbarButton* WebAppFrameToolbarView::GetDownloadButton() {
  return right_container_ ? right_container_->GetDownloadButton() : nullptr;
}
