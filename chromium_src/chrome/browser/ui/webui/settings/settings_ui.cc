/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_import_data_handler.h"
#include "brave/browser/ui/webui/settings/brave_search_engines_handler.h"
#include "brave/browser/ui/webui/settings/brave_site_settings_handler.h"
#include "brave/browser/ui/webui/settings/settings_cookies_view_handler.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "chrome/browser/ui/webui/settings/hats_handler.h"
#include "chrome/browser/ui/webui/settings/settings_secure_dns_handler.h"
#include "chrome/browser/ui/webui/settings/site_settings_handler.h"

#if BUILDFLAG(IS_WIN) && BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/webui/settings/brave_settings_secure_dns_handler.h"

#define SecureDnsHandler BraveSecureDnsHandler
#endif
#define SiteSettingsHandler BraveSiteSettingsHandler
#define ImportDataHandler BraveImportDataHandler
#define SearchEnginesHandler BraveSearchEnginesHandler
#define HatsHandler HatsHandler>());AddSettingsPageUIHandler(std::make_unique<CookiesViewHandler
#include "src/chrome/browser/ui/webui/settings/settings_ui.cc"
#undef HatsHandler
#undef ImportDataHandler
#undef SearchEnginesHandler
#undef SiteSettingsHandler
