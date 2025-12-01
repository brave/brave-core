/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/layout/browser_view_layout_impl_old.h"

#include "brave/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout_delegate.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"

// Double check if the tab strip is actually visible to calculate constraints.
#define SupportsWindowFeature(FEATURE)                                       \
  SupportsWindowFeature(FEATURE) && (FEATURE != Browser::FEATURE_TABSTRIP || \
                                     delegate().ShouldDrawTabStrip());

#include <chrome/browser/ui/views/frame/layout/browser_view_layout_impl_old.cc>

#undef SupportsWindowFeature
