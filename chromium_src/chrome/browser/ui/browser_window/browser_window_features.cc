/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"

#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"

#define SidePanelCoordinator BraveSidePanelCoordinator
#include "src/chrome/browser/ui/browser_window/browser_window_features.cc"
#undef SidePanelCoordinator
