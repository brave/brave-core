/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"

#include "ui/compositor/compositor.h"

#if BUILDFLAG(IS_WIN)
// This header includes shobjidl.h which also defines SetBackgroundColor in
// Windows SDK 10.0.26100.0
#include "ui/gfx/win/hwnd_util.h"
#endif

#include <ui/views/widget/desktop_aura/desktop_native_widget_aura.cc>
