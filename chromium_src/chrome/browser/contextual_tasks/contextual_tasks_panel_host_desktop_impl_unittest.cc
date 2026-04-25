// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_MOCK_SIDE_PANEL_UI_TOGGLE \
  MOCK_METHOD(void, Toggle, (), (override));

#include <chrome/browser/contextual_tasks/contextual_tasks_panel_host_desktop_impl_unittest.cc>

#undef BRAVE_MOCK_SIDE_PANEL_UI_TOGGLE
