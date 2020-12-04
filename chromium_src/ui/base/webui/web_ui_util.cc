// Copyright (c) 2019 The Brave Authors
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "ui/base/webui/web_ui_util.h"

#include "brave/ui/webui/resources/grit/brave_webui_resources.h"
#include "ui/resources/grit/webui_resources.h"

// Replace text_defaults.css with brave's text_defaults.css
// which is defined in brave_webui_resources.grd.
#define IDR_WEBUI_CSS_TEXT_DEFAULTS_MD_CSS_PREVIOUS \
  IDR_WEBUI_CSS_TEXT_DEFAULTS_MD_CSS

#undef IDR_WEBUI_CSS_TEXT_DEFAULTS_MD_CSS
#define IDR_WEBUI_CSS_TEXT_DEFAULTS_MD_CSS IDR_BRAVE_WEBUI_CSS_TEXT_DEFAULTS

#include "../../../../../ui/base/webui/web_ui_util.cc"

#undef IDR_WEBUI_CSS_TEXT_DEFAULTS_MD_CSS
#define IDR_WEBUI_CSS_TEXT_DEFAULTS_MD_CSS \
  IDR_WEBUI_CSS_TEXT_DEFAULTS_MD_CSS_PREVIOUS
