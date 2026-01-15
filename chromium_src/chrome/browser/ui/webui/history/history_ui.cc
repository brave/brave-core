/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// No Brave-specific customizations needed for history UI after removing
// navigation bar data provider.

#define BRAVE_CREATE_HISTORY_UI_HTML_SOURCE

#include <chrome/browser/ui/webui/history/history_ui.cc>
#undef BRAVE_CREATE_HISTORY_UI_HTML_SOURCE
