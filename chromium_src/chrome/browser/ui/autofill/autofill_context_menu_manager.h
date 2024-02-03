// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_AUTOFILL_AUTOFILL_CONTEXT_MENU_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_AUTOFILL_AUTOFILL_CONTEXT_MENU_MANAGER_H_

#define AppendItems           \
  AppendItems_ChromiumImpl(); \
  void AppendItems

#include "src/chrome/browser/ui/autofill/autofill_context_menu_manager.h"  // IWYU pragma: export
#undef AppendItems

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_AUTOFILL_AUTOFILL_CONTEXT_MENU_MANAGER_H_
