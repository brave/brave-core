/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <type_traits>

#include "brave/browser/ui/views/frame/brave_opaque_browser_frame_view.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/ui/views/frame/brave_browser_frame_view_win.h"
#endif

#if BUILDFLAG(IS_LINUX)
#include "brave/browser/ui/views/frame/brave_browser_frame_view_linux_native.h"
#endif

#include <chrome/browser/ui/views/frame/browser_frame_view_factory_views.cc>

#if BUILDFLAG(IS_LINUX)
static_assert(
    std::is_same_v<std::unique_ptr<BraveOpaqueBrowserFrameView>,
                   std::invoke_result_t<
                       decltype(chrome::CreateOpaqueBrowserFrameViewLinux),
                       BrowserWidget*,
                       BrowserView*>>,
    "CreateOpaqueBrowserFrameViewLinux is not returning "
    "BraveOpaqueBrowserFrameView");
#endif  // BUILDFLAG(IS_LINUX)
