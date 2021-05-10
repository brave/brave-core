/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_METRICS_REPORTING_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_METRICS_REPORTING_HANDLER_H_

#include "build/branding_buildflags.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/policy/core/common/policy_service.h"
#include "components/prefs/pref_member.h"

#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)
#include "../../../../../../../chrome/browser/ui/webui/settings/metrics_reporting_handler.h"
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (0)

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_METRICS_REPORTING_HANDLER_H_
