/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ui/views/bubble/bubble_dialog_delegate_view.h"  // IWYU pragma: keep

#define SetExtraView(...)    \
  SetExtraView(__VA_ARGS__); \
  SetFootnoteView(device_chooser_content_view_->CreateFootnoteView(browser))

#include <chrome/browser/ui/views/permissions/chooser_bubble_ui.cc>

#undef SetExtraView
