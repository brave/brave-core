/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_about_handler.h"
#include "brave/browser/ui/webui/settings/brave_settings_import_data_handler.h"
#include "chrome/browser/ui/webui/settings/about_handler.h"

#define AboutHandler BraveAboutHandler
#define ImportDataHandler BraveImportDataHandler
#include "../../../../../../../chrome/browser/ui/webui/settings/settings_ui.cc"  // NOLINT
#undef AboutHandler
#undef ImportDataHandler
