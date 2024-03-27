/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#if defined(OFFICIAL_BUILD)
#define BRAVE_DEV_TOOLS_UI_BINDINGS_ADD_DEV_TOOLS_EXTENSIONS_TO_CLIENT
#else
#define BRAVE_DEV_TOOLS_UI_BINDINGS_ADD_DEV_TOOLS_EXTENSIONS_TO_CLIENT \
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(               \
          "extensions-on-chrome-urls")) {                              \
    CallClientMethod("DevToolsAPI", "setEnabledOnChromeUrls");         \
  }
#endif  // defined(OFFICIAL_BUILD)

#include "src/chrome/browser/devtools/devtools_ui_bindings.cc"

#undef BRAVE_DEV_TOOLS_UI_BINDINGS_ADD_DEV_TOOLS_EXTENSIONS_TO_CLIENT
