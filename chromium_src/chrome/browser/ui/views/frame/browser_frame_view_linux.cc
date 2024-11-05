/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/browser_frame_view_linux.h"
#include "brave/browser/ui/views/frame/brave_opaque_browser_frame_view.h"

#define OpaqueBrowserFrameView BraveOpaqueBrowserFrameView
#include "src/chrome/browser/ui/views/frame/browser_frame_view_linux.cc"
#undef OpaqueBrowserFrameView
