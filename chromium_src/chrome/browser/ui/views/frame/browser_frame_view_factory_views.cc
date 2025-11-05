/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <type_traits>

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/ui/views/frame/brave_browser_frame_view_win.h"
#define BrowserFrameViewWin BraveBrowserFrameViewWin
#endif

// This file is included for all platform by upstream source code and we need
// to include this first to avoid #define conflicts.
#include "brave/browser/ui/views/frame/brave_opaque_browser_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_frame_view_linux.h"
#define OpaqueBrowserFrameView BraveOpaqueBrowserFrameView

#if BUILDFLAG(IS_LINUX)
#include "brave/browser/ui/views/frame/brave_browser_frame_view_linux_native.h"
#define BrowserFrameViewLinuxNative BraveBrowserFrameViewLinuxNative
#endif  // BUILDFLAG(IS_LINUX)

#include <chrome/browser/ui/views/frame/browser_frame_view_factory_views.cc>

#undef OpaqueBrowserFrameView

#if BUILDFLAG(IS_LINUX)
#undef BrowserFrameViewLinuxNative

// A sanity check for our macro
static_assert(
    std::is_same_v<std::unique_ptr<BraveOpaqueBrowserFrameView>,
                   std::invoke_result_t<
                       decltype(chrome::CreateOpaqueBrowserFrameViewLinux),
                       BrowserWidget*,
                       BrowserView*>>,
    "CreateOpaqueBrowserFrameViewLinux is not returning "
    "BraveOpaqueBrowserFrameView");
#endif  // BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_WIN)
#undef BrowserFrameViewWin
#endif
