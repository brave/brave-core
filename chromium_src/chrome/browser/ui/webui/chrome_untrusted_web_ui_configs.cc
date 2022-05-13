/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/chrome_untrusted_web_ui_configs.h"

#include "brave/browser/ui/webui/brave_wallet/trezor/trezor_ui.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "build/build_config.h"
#include "content/public/browser/webui_config_map.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN) && !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/webui/brave_vpn/vpn_panel_ui.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#endif

#define RegisterChromeUntrustedWebUIConfigs \
  RegisterChromeUntrustedWebUIConfigs_ChromiumImpl

#include "src/chrome/browser/ui/webui/chrome_untrusted_web_ui_configs.cc"

#undef RegisterChromeUntrustedWebUIConfigs

void RegisterChromeUntrustedWebUIConfigs() {
  RegisterChromeUntrustedWebUIConfigs_ChromiumImpl();
#if !BUILDFLAG(IS_ANDROID)
  content::WebUIConfigMap::GetInstance().AddUntrustedWebUIConfig(
      std::make_unique<trezor::UntrustedTrezorUIConfig>());
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (brave_vpn::IsBraveVPNEnabled()) {
    content::WebUIConfigMap::GetInstance().AddUntrustedWebUIConfig(
        std::make_unique<UntrustedVPNPanelUIConfig>());
  }
#endif
#endif
}
