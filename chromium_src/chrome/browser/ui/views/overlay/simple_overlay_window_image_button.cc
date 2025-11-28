// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/display/display.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/view.h"

// Try to override icon size calculation in
// SimpleOverlayWindowImageButton::UpdateImage().
#define width() icon_size_.value_or(width())

#include <chrome/browser/ui/views/overlay/simple_overlay_window_image_button.cc>

#undef width
