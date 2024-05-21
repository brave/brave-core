/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "ui/base/ui_base_features.h"

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
namespace {
bool ShouldTabSearchAppearBeforeTabStrip() {
  return false;
}
}  // namespace

#define IsChromeRefresh2023 \
  IsChromeRefresh2023() && ShouldTabSearchAppearBeforeTabStrip
#endif

#include "src/chrome/browser/ui/views/tab_search_bubble_host.cc"

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
#undef IsChromeRefresh2023
#endif
