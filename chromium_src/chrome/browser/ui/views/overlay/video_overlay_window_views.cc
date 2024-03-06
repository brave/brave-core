/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/overlay/brave_back_to_tab_label_button.h"
#include "brave/browser/ui/views/overlay/brave_video_overlay_window_views.h"

// Also has WrapUnique.
#include "cc/trees/task_runner_provider.h"
#include "content/public/browser/web_contents_user_data.h"

#define BRAVE_UPDATE_MAX_SIZE max_size_ = work_area.size();
#define WrapUnique(VideoOverlayWindowViews) \
  WrapUnique(new BraveVideoOverlayWindowViews(controller));

#define BackToTabLabelButton BraveBackToTabLabelButton

#include "src/chrome/browser/ui/views/overlay/video_overlay_window_views.cc"

#undef BackToTabLabelButton
#undef WrapUnique
#undef BRAVE_UPDATE_MAX_SIZE
