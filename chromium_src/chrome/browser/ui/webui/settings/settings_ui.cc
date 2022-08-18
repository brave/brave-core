/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_import_data_handler.h"
#include "brave/browser/ui/webui/settings/brave_search_engines_handler.h"
#include "brave/browser/ui/webui/settings/brave_settings_default_browser_handler.h"

#define ImportDataHandler BraveImportDataHandler
#define SearchEnginesHandler BraveSearchEnginesHandler
#define DefaultBrowserHandler BraveDefaultBrowserHandler
#include "src/chrome/browser/ui/webui/settings/settings_ui.cc"
#undef DefaultBrowserHandler
#undef ImportDataHandler
#undef SearchEnginesHandler
