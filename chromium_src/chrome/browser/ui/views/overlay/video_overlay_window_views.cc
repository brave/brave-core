/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/overlay/brave_video_overlay_window_views.h"

#include "build/build_config.h"

#define BRAVE_UPDATE_MAX_SIZE max_size_ = work_area.size();

#if BUILDFLAG(IS_LINUX)
// Expands in VideoOverlayWindowViews::Create() right before the widget is
// initialized, to give the PiP window a WM_CLASS / app id on Linux.
#define BRAVE_VIDEO_OVERLAY_WINDOW_VIEWS_CREATE \
  BraveVideoOverlayWindowViews::SetLinuxWMClass(params, controller);
#else
#define BRAVE_VIDEO_OVERLAY_WINDOW_VIEWS_CREATE
#endif

#include <chrome/browser/ui/views/overlay/video_overlay_window_views.cc>

#undef BRAVE_VIDEO_OVERLAY_WINDOW_VIEWS_CREATE
#undef BRAVE_UPDATE_MAX_SIZE
