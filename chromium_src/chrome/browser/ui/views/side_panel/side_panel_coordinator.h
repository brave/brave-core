/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_COORDINATOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_COORDINATOR_H_

// Moved all header includes of upstream side_panel_coordinator.h to apply
// final define only to side_panel_coordinator.h as the final keyword is very
// commonly used from many places.
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_multi_source_observation.h"
#include "base/scoped_observation_traits.h"
#include "base/time/time.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/views/side_panel/side_panel_util.h"
#include "ui/views/view_observer.h"

#define CreateHeader                      \
  CreateHeader_UnUsed() {                 \
    return nullptr;                       \
  }                                       \
  friend class BraveSidePanelCoordinator; \
  virtual std::unique_ptr<views::View> CreateHeader

#define NotifyPinnedContainerOfActiveStateChange \
  virtual NotifyPinnedContainerOfActiveStateChange

#define PopulateSidePanel virtual PopulateSidePanel

#include "src/chrome/browser/ui/views/side_panel/side_panel_coordinator.h"  // IWYU pragma: export

#undef PopulateSidePanel
#undef NotifyPinnedContainerOfActiveStateChange
#undef CreateHeader

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_COORDINATOR_H_
