/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/chrome_web_ui_configs.h"

#include "content/public/browser/webui_config_map.h"

#define RegisterChromeWebUIConfigs RegisterChromeWebUIConfigs_ChromiumImpl

#include "src/chrome/browser/ui/webui/chrome_web_ui_configs.cc"
#undef RegisterChromeWebUIConfigs

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/webui/brave_rewards/rewards_page_top_ui.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_panel_ui.h"
#include "brave/browser/ui/webui/brave_rewards/tip_panel_ui.h"
#include "brave/browser/ui/webui/brave_shields/cookie_list_opt_in_ui.h"
#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#include "brave/browser/ui/webui/private_new_tab_page/brave_private_new_tab_ui.h"
#include "brave/browser/ui/webui/speedreader/speedreader_toolbar_ui.h"
#include "brave/browser/ui/webui/webcompat_reporter/webcompat_reporter_ui.h"
#endif  // !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/webui/brave_adblock_internals_ui.h"
#include "brave/browser/ui/webui/brave_adblock_ui.h"

namespace {

#if !BUILDFLAG(IS_ANDROID)
const GURL GetWebUIConfigURL(const char* scheme, const char* host) {
  return GURL(base::StrCat({scheme, url::kStandardSchemeSeparator, host}));
}
#endif  // !BUILDFLAG(IS_ANDROID)

void RemoveOverridenWebUIs(content::WebUIConfigMap& map) {
#if !BUILDFLAG(IS_ANDROID)
  // Remove NewTabUIConfig. It will be replaced with BravePrivateNewTabUIConfig.
  // Note that this only handles new tab for private profiles (Private, Tor,
  // Guest). For regular profile the handling is still done in
  // BraveWebUIControllerFactory. We will need to follow up on transitioning
  // BraveNewTabUI to using WebUIConfig. Currently, we can't add both
  // BravePrivateNewTabUI and BraveNewTabUI configs to the map because they
  // use the same origin (content::kChromeUIScheme +
  // chrome::kChromeUINewTabHost).
  map.RemoveConfig(
      GetWebUIConfigURL(content::kChromeUIScheme, chrome::kChromeUINewTabHost));
#endif  // !BUILDFLAG(IS_ANDROID)
}

}  // namespace

void RegisterChromeWebUIConfigs() {
  RegisterChromeWebUIConfigs_ChromiumImpl();

  auto& map = content::WebUIConfigMap::GetInstance();
  // Remove configs for Chromium WebUIs that we replace with our own WebUIs.
  // The map doesn't allow for multiple entries for the same origin, so the
  // upstream configs must be removed before we can add our own configs.
  RemoveOverridenWebUIs(map);

#if !BUILDFLAG(IS_ANDROID)
  map.AddWebUIConfig(std::make_unique<brave_rewards::RewardsPageTopUIConfig>());
  map.AddWebUIConfig(std::make_unique<brave_rewards::RewardsPanelUIConfig>());
  map.AddWebUIConfig(std::make_unique<brave_rewards::TipPanelUIConfig>());
  map.AddWebUIConfig(std::make_unique<BravePrivateNewTabUIConfig>());
  map.AddWebUIConfig(std::make_unique<CookieListOptInUIConfig>());
  map.AddWebUIConfig(std::make_unique<ShieldsPanelUIConfig>());
  map.AddWebUIConfig(std::make_unique<SpeedreaderToolbarUIConfig>());
  map.AddWebUIConfig(std::make_unique<WalletPanelUIConfig>());
  map.AddWebUIConfig(
      std::make_unique<webcompat_reporter::WebcompatReporterUIConfig>());
#endif  // !BUILDFLAG(IS_ANDROID)
  map.AddWebUIConfig(std::make_unique<BraveAdblockUIConfig>());
  map.AddWebUIConfig(std::make_unique<BraveAdblockInternalsUIConfig>());
}
