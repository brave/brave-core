/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"

#include "ui/compositor/compositor.h"

#define SetBackgroundColor(color) \
  SetBackgroundColor(desktop_window_tree_host_->GetBackgroundColor(color))

#include "src/ui/views/widget/desktop_aura/desktop_native_widget_aura.cc"

#undef SetBackgroundColor
