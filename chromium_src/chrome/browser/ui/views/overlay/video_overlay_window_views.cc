/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/overlay/brave_back_to_tab_label_button.h"
#include "brave/browser/ui/views/overlay/brave_video_overlay_window_views.h"

#define BRAVE_UPDATE_MAX_SIZE max_size_ = work_area.size();
#define BRAVE_CREATE        \
  overlay_window->Close();  \
  overlay_window.release(); \
  overlay_window = std::make_unique<BraveVideoOverlayWindowViews>(controller);
#define BackToTabLabelButton BraveBackToTabLabelButton

#include "src/chrome/browser/ui/views/overlay/video_overlay_window_views.cc"

#undef BackToTabLabelButton
#undef BRAVE_CREATE
#undef BRAVE_UPDATE_MAX_SIZE
