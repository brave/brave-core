/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/settings_localized_strings_provider.h"

#include "brave/browser/ui/webui/settings/brave_settings_localized_strings_provider.h"
#include "brave/components/version_info/version_info.h"
#include "brave/grit/brave_generated_resources.h"

// Override some chromium strings
#include "chrome/grit/branded_strings.h"
#include "chrome/grit/generated_resources.h"

#undef IDS_SETTINGS_SEARCH_EXPLANATION
#define IDS_SETTINGS_SEARCH_EXPLANATION \
  IDS_SETTINGS_NORMAL_PROFILE_SEARCH_EXPLANATION

#undef IDS_SETTINGS_CUSTOMIZE_PROFILE
#define IDS_SETTINGS_CUSTOMIZE_PROFILE IDS_SETTINGS_BRAVE_EDIT_PROFILE

#undef IDS_SETTINGS_CUSTOMIZE_YOUR_CHROME_PROFILE
#define IDS_SETTINGS_CUSTOMIZE_YOUR_CHROME_PROFILE \
  IDS_SETTINGS_BRAVE_EDIT_PROFILE

#undef IDS_SETTINGS_SAFEBROWSING_STANDARD_BULLET_TWO
#define IDS_SETTINGS_SAFEBROWSING_STANDARD_BULLET_TWO \
  IDS_SETTINGS_BRAVE_SAFEBROWSING_STANDARD_BULLET_TWO

#undef IDS_SETTINGS_SAFEBROWSING_NONE_DESC
#define IDS_SETTINGS_SAFEBROWSING_NONE_DESC \
  IDS_SETTINGS_BRAVE_SAFEBROWSING_NONE_DESC

// Use custom strings for diagnostic (crashes, hangs) reporting settings.
#undef IDS_SETTINGS_ENABLE_LOGGING_PREF
#undef IDS_SETTINGS_ENABLE_LOGGING_PREF_DESC
#define IDS_SETTINGS_ENABLE_LOGGING_PREF IDS_BRAVE_DIAGNOSTIC_REPORTS_PREF
#define IDS_SETTINGS_ENABLE_LOGGING_PREF_DESC \
  IDS_BRAVE_DIAGNOSTIC_REPORTS_PREF_DESC

// Sync types labels
#undef IDS_SETTINGS_AUTOFILL_CHECKBOX_LABEL
#define IDS_SETTINGS_AUTOFILL_CHECKBOX_LABEL \
  IDS_SETTINGS_BRAVE_AUTOFILL_CHECKBOX_LABEL

#undef IDS_SETTINGS_APPS_CHECKBOX_LABEL
#define IDS_SETTINGS_APPS_CHECKBOX_LABEL IDS_SETTINGS_BRAVE_APPS_CHECKBOX_LABEL

#include "src/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc"
#undef IDS_SETTINGS_APPS_CHECKBOX_LABEL
#undef IDS_SETTINGS_AUTOFILL_CHECKBOX_LABEL
#undef IDS_SETTINGS_ENABLE_LOGGING_PREF_DESC
#undef IDS_SETTINGS_ENABLE_LOGGING_PREF
#undef IDS_SETTINGS_SAFEBROWSING_NONE_DESC
#undef IDS_SETTINGS_SAFEBROWSING_STANDARD_BULLET_TWO
#undef IDS_SETTINGS_CUSTOMIZE_YOUR_CHROME_PROFILE
#undef IDS_SETTINGS_CUSTOMIZE_PROFILE
#undef IDS_SETTINGS_SEARCH_EXPLANATION
