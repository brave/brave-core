/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_untrusted_web_ui_controller_factory.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/no_destructor.h"
#include "brave/browser/ui/webui/brave_wallet/trezor/trezor_ui.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "build/build_config.h"
#include "ui/webui/webui_config.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN) && !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/webui/brave_vpn/vpn_panel_ui.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#endif

using WebUIConfigList =
    std::vector<std::pair<std::string, std::unique_ptr<ui::WebUIConfig>>>;

namespace {

WebUIConfigList CreateConfigs() {
  WebUIConfigList config_list;
  auto register_config =
      [&config_list](std::unique_ptr<ui::WebUIConfig> config) {
        DCHECK_EQ(config->scheme(), content::kChromeUIUntrustedScheme);
        const std::string& host = config->host();
        config_list.emplace_back(host, std::move(config));
      };
  register_config(std::make_unique<trezor::UntrustedTrezorUIConfig>());

#if BUILDFLAG(ENABLE_BRAVE_VPN) && !BUILDFLAG(IS_ANDROID)
  if (brave_vpn::IsBraveVPNEnabled()) {
    register_config(std::make_unique<UntrustedVPNPanelUIConfig>());
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

  return config_list;
}

}  // namespace

// static
void BraveUntrustedWebUIControllerFactory::RegisterInstance() {
  static base::NoDestructor<BraveUntrustedWebUIControllerFactory> instance;
  content::WebUIControllerFactory::RegisterFactory(instance.get());
}

BraveUntrustedWebUIControllerFactory::BraveUntrustedWebUIControllerFactory()
    : configs_(CreateConfigs()) {}

BraveUntrustedWebUIControllerFactory::~BraveUntrustedWebUIControllerFactory() =
    default;

const ui::UntrustedWebUIControllerFactory::WebUIConfigMap&
BraveUntrustedWebUIControllerFactory::GetWebUIConfigMap() {
  return configs_;
}
