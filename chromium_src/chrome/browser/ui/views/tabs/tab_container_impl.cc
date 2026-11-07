/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_container_impl.h"

#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"

namespace {

// Brave uses a smaller window drag handle extension for compact horizontal
// tabs. Upstream used to expose this through
// TabStyle::GetDragHandleExtension(), which we overrode, but that method was
// removed and its value inlined as a constant, so we adjust it here at its
// point of use instead.
int GetBraveDragHandleExtension(int drag_handle_extension) {
  if (!tabs::HorizontalTabsUpdateEnabled()) {
    return drag_handle_extension;
  }
  return tabs::GetDragHandleExtensionHeight();
}

}  // namespace

#include <chrome/browser/ui/views/tabs/tab_container_impl.cc>
