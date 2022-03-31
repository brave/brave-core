/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/ui/views/frame/brave_glass_browser_frame_view.h"
#define GlassBrowserFrameView BraveGlassBrowserFrameView
#endif

#include "brave/browser/ui/views/frame/brave_opaque_browser_frame_view.h"
#define OpaqueBrowserFrameView BraveOpaqueBrowserFrameView

#include "src/chrome/browser/ui/views/frame/browser_non_client_frame_view_factory_views.cc"

#undef OpaqueBrowserFrameView

#if BUILDFLAG(IS_WIN)
#undef GlassBrowserFrameView
#endif
