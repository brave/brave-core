// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_UI_RESOURCES_BRAVE_OVERRIDES_H_
#define BRAVE_UI_RESOURCES_BRAVE_OVERRIDES_H_

#include "ui/resources/grit/webui_resources.h"

#undef IDR_WEBUI_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_HTML_JS
#undef IDR_WEBUI_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_JS
#define IDR_WEBUI_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_HTML_JS IDR_BRAVE_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_HTML_JS
#define IDR_WEBUI_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_JS IDR_BRAVE_CR_ELEMENTS_CR_BUTTON_CR_BUTTON_JS

#endif  // BRAVE_UI_RESOURCES_BRAVE_OVERRIDES_H_
