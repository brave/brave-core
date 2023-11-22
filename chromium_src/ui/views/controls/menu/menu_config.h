// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_MENU_MENU_CONFIG_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_MENU_MENU_CONFIG_H_

#include "ui/gfx/font_list.h"
#include "ui/views/style/typography_provider.h"

// Added default copy ctor as defining instance gives below chromium style
// complains. "Complex class/struct needs an explicit out-of-line copy
// constructor"
#define instance                 \
  instance_ChromiumImpl();       \
  MenuConfig(const MenuConfig&); \
  static const MenuConfig& instance

#include "src/ui/views/controls/menu/menu_config.h"  // IWYU pragma: export

#undef instance

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_MENU_MENU_CONFIG_H_
