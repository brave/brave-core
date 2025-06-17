// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "content/public/browser/web_ui_data_source.h"

#define AddLocalizedStrings(...)                               \
  AddLocalizedStrings(__VA_ARGS__);                            \
  source->AddLocalizedString("braveCustomizeMenuToolbarLabel", \
                             IDS_BRAVE_CUSTOMIZE_MENU_TOOLBAR_LABEL)

#include <chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_ui.cc>

#undef AddLocalizedStrings
