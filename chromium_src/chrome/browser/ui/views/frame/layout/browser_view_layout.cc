/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout_impl_old.h"

#define BrowserViewLayoutImplOld BraveBrowserViewLayout

#include <chrome/browser/ui/views/frame/layout/browser_view_layout.cc>
#undef BrowserViewLayoutImplOld

void BrowserViewLayout::NotifyDialogPositionRequiresUpdate() {
  dialog_host_->NotifyPositionRequiresUpdate();
}
