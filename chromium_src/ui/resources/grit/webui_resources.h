// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_RESOURCES_GRIT_WEBUI_RESOURCES_H_
#define BRAVE_CHROMIUM_SRC_UI_RESOURCES_GRIT_WEBUI_RESOURCES_H_

#include "build/build_config.h"

#include "../gen/ui/resources/grit/webui_resources.h"  // IWYU pragma: export

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
// Override cr-button HTML
#ifdef IDR_WEBUI_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_HTML_JS
#undef IDR_WEBUI_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_HTML_JS
#define IDR_WEBUI_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_HTML_JS \
  IDR_BRAVE_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_HTML_JS
#else
#error IDR_WEBUI_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_HTML_JS is not defined upstream
#endif

// Override cr-button JS
#ifdef IDR_WEBUI_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_JS
#undef IDR_WEBUI_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_JS
#define IDR_WEBUI_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_JS \
  IDR_BRAVE_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_JS
#else
#error IDR_WEBUI_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_JS is not defined upstream
#endif

// Override cr-toggle HTML
#ifdef IDR_WEBUI_CR_ELEMENTS_CR_TOGGLE_CR_TOGGLE_HTML_JS
#undef IDR_WEBUI_CR_ELEMENTS_CR_TOGGLE_CR_TOGGLE_HTML_JS
#define IDR_WEBUI_CR_ELEMENTS_CR_TOGGLE_CR_TOGGLE_HTML_JS \
  IDR_BRAVE_CR_ELEMENTS_CR_TOGGLE_CR_TOGGLE_HTML_JS
#else
#error IDR_WEBUI_CR_ELEMENTS_CR_TOGGLE_CR_TOGGLE_HTML_JS is not defined upstream
#endif

// Override cr-toggle JS
#ifdef IDR_WEBUI_CR_ELEMENTS_CR_TOGGLE_CR_TOGGLE_JS
#undef IDR_WEBUI_CR_ELEMENTS_CR_TOGGLE_CR_TOGGLE_JS
#define IDR_WEBUI_CR_ELEMENTS_CR_TOGGLE_CR_TOGGLE_JS \
  IDR_BRAVE_CR_ELEMENTS_CR_TOGGLE_CR_TOGGLE_JS
#else
#error IDR_WEBUI_CR_ELEMENTS_CR_TOGGLE_CR_TOGGLE_JS is not defined upstream
#endif
#endif

#endif  // BRAVE_CHROMIUM_SRC_UI_RESOURCES_GRIT_WEBUI_RESOURCES_H_
