/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_view_util.h"

// static
int BraveContentsViewUtil::GetBorderRadius() {
  return 6;
}

int BraveContentsViewUtil::GetBorderRadiusAroundWindow() {
  if (@available(macOS 26, *)) {
    return 17;
  }

  return 6;
}
