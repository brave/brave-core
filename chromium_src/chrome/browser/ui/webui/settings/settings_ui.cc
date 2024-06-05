/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/settings_ui.h"

#include "brave/browser/ui/webui/cr_components/theme_color_picker/brave_theme_color_picker_handler.h"
#include "brave/browser/ui/webui/settings/brave_import_data_handler.h"
#include "brave/browser/ui/webui/settings/brave_search_engines_handler.h"
#include "brave/browser/ui/webui/settings/brave_site_settings_handler.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "chrome/browser/ui/webui/settings/hats_handler.h"
#include "chrome/browser/ui/webui/settings/settings_secure_dns_handler.h"
#include "chrome/browser/ui/webui/settings/site_settings_handler.h"

#if BUILDFLAG(IS_WIN) && BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/webui/settings/brave_settings_secure_dns_handler.h"

#define SecureDnsHandler BraveSecureDnsHandler
#endif  // BUILDFLAG(IS_WIN) && BUILDFLAG(ENABLE_BRAVE_VPN)

#define SiteSettingsHandler BraveSiteSettingsHandler
#define ImportDataHandler BraveImportDataHandler
#define SearchEnginesHandler BraveSearchEnginesHandler
#define CreateThemeColorPickerHandler CreateThemeColorPickerHandler_Unused
#include "src/chrome/browser/ui/webui/settings/settings_ui.cc"
#undef CreateThemeColorPickerHandler
#undef SearchEnginesHandler
#undef ImportDataHandler
#undef SiteSettingsHandler
#if BUILDFLAG(IS_WIN) && BUILDFLAG(ENABLE_BRAVE_VPN)
#undef SecureDnsHandler
#endif  // BUILDFLAG(IS_WIN) && BUILDFLAG(ENABLE_BRAVE_VPN)

namespace settings {
void SettingsUI::CreateThemeColorPickerHandler(
    mojo::PendingReceiver<theme_color_picker::mojom::ThemeColorPickerHandler>
        handler,
    mojo::PendingRemote<theme_color_picker::mojom::ThemeColorPickerClient>
        client) {
  theme_color_picker_handler_ = std::make_unique<BraveThemeColorPickerHandler>(
      std::move(handler), std::move(client),
      NtpCustomBackgroundServiceFactory::GetForProfile(
          Profile::FromWebUI(web_ui())),
      web_ui()->GetWebContents());
}
}  // namespace settings
