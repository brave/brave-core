/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/browser/ui/views/tabs/brave_tab_container.h"
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"
#include "brave/browser/ui/views/tabs/brave_vertical_tab_utils.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_container_impl.h"

#if BUILDFLAG(IS_WIN)
#include "ui/gfx/win/hwnd_util.h"
#endif

#define AddTab(TAB, MODEL_INDEX, PINNED) \
  AddTab(std::make_unique<BraveTab>(this), MODEL_INDEX, PINNED)
#define TabHoverCardController BraveTabHoverCardController
#define TabContainer BraveTabContainer

#include "src/chrome/browser/ui/views/tabs/tab_strip.cc"

#undef TabContainer
#undef TabHoverCardController
#undef AddTab
