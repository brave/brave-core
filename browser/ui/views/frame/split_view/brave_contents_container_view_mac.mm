/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"

float BraveContentsContainerView::GetCornerRadiusWithoutRoundedCorners() const {
  if (!is_in_split_) {
    return 0;
  }

  if (@available(macOS 26, *)) {
    return 20;
  }

  return 0;
}
