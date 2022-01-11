/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/grit/generated_resources.h"

// Use custom strings for diagnostic (crashes, hangs) reporting settings.
#undef IDS_SETTINGS_ENABLE_LOGGING_PREF
#undef IDS_SETTINGS_ENABLE_LOGGING_PREF_DESC
#define IDS_SETTINGS_ENABLE_LOGGING_PREF IDS_BRAVE_DIAGNOSTIC_REPORTS_PREF
#define IDS_SETTINGS_ENABLE_LOGGING_PREF_DESC \
  IDS_BRAVE_DIAGNOSTIC_REPORTS_PREF_DESC

#include "src/chrome/browser/ui/webui/settings/shared_settings_localized_strings_provider.cc"
