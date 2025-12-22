// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_MOCK_SIDE_PANEL_UI_TOGGLE \
  MOCK_METHOD(void, Toggle, (), (override));

#include <chrome/browser/contextual_tasks/contextual_tasks_side_panel_coordinator_unittest.cc>

#undef BRAVE_MOCK_SIDE_PANEL_UI_TOGGLE
