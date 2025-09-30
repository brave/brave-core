/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_view_util.h"

// static
int BraveContentsViewUtil::GetMargin() {
  if (@available(macOS 26, *)) {
    return 8;
  }
  return 4;
}

// static
int BraveContentsViewUtil::GetBorderRadius() {
  if (@available(macOS 26, *)) {
    return 18;
  }
  return 6;
}
