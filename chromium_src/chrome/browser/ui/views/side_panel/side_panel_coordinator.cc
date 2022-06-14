// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Brave has its own side panel navigation in the form of the SideBar, so
// hide the Chromium combobox-style header.
#define BRAVE_SIDE_PANEL_COORDINATOR_CREATE_HEADER header->SetVisible(false);
#include "src/chrome/browser/ui/views/side_panel/side_panel_coordinator.cc"
#undef BRAVE_SIDE_PANEL_COORDINATOR_CREATE_HEADER
