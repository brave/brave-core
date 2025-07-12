// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_CR_COMPONENTS_CUSTOMIZE_COLOR_SCHEME_MODE_CUSTOMIZE_COLOR_SCHEME_MODE_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_CR_COMPONENTS_CUSTOMIZE_COLOR_SCHEME_MODE_CUSTOMIZE_COLOR_SCHEME_MODE_HANDLER_H_

#define UpdateColorSchemeMode                        \
  UpdateColorSchemeMode_Unused();                    \
  friend class BraveCustomizeColorSchemeModeHandler; \
  virtual void UpdateColorSchemeMode

#include "src/chrome/browser/ui/webui/cr_components/customize_color_scheme_mode/customize_color_scheme_mode_handler.h"  // IWYU pragma: export

#undef UpdateColorSchemeMode

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_CR_COMPONENTS_CUSTOMIZE_COLOR_SCHEME_MODE_CUSTOMIZE_COLOR_SCHEME_MODE_HANDLER_H_
