/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ui/base/webui/web_ui_util.h"

#include "build/build_config.h"
#include "ui/resources/grit/webui_resources.h"

#if !BUILDFLAG(IS_IOS)
// Replace text_defaults_md.css with brave's text_defaults_md.css
// which is defined in brave_webui_resources.grd.
#undef IDR_WEBUI_CSS_TEXT_DEFAULTS_MD_CSS
#define IDR_WEBUI_CSS_TEXT_DEFAULTS_MD_CSS \
  IDR_BRAVE_WEBUI_CSS_TEXT_DEFAULTS_MD_CSS
#endif

#include "src/ui/base/webui/web_ui_util.cc"

#if !BUILDFLAG(IS_IOS)
#undef IDR_WEBUI_CSS_TEXT_DEFAULTS_MD_CSS
#endif
