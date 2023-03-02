// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_settings_ui.h"

#include <memory>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/browser/resources/settings/grit/brave_settings_resources.h"
#include "brave/browser/resources/settings/grit/brave_settings_resources_map.h"
#include "brave/browser/shell_integrations/buildflags/buildflags.h"
#include "brave/browser/ui/commands/accelerator_service_factory.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "brave/browser/ui/webui/settings/brave_adblock_handler.h"
#include "brave/browser/ui/webui/settings/brave_appearance_handler.h"
#include "brave/browser/ui/webui/settings/brave_default_extensions_handler.h"
#include "brave/browser/ui/webui/settings/brave_privacy_handler.h"
#include "brave/browser/ui/webui/settings/brave_sync_handler.h"
#include "brave/browser/ui/webui/settings/brave_wallet_handler.h"
#include "brave/browser/ui/webui/settings/default_brave_shields_handler.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/version_info/version_info.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/settings/metrics_reporting_handler.h"
#include "components/sync/base/command_line_switches.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/content_features.h"
#include "extensions/buildflags/buildflags.h"
#include "net/base/features.h"

#if BUILDFLAG(ENABLE_PIN_SHORTCUT)
#include "brave/browser/ui/webui/settings/pin_shortcut_handler.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/common/features.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/vpn_utils.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/ui/webui/settings/brave_tor_handler.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/ui/webui/settings/brave_extensions_manifest_v2_handler.h"
#include "brave/browser/ui/webui/settings/brave_tor_snowflake_extension_handler.h"
#endif

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
#include "brave/components/commands/browser/resources/grit/commands_generated_map.h"
#include "brave/components/commands/common/commands.mojom.h"
#include "brave/components/commands/common/features.h"
#endif

using ntp_background_images::ViewCounterServiceFactory;

BraveSettingsUI::BraveSettingsUI(content::WebUI *web_ui,
                                 const std::string &host)
    : SettingsUI(web_ui) {
  web_ui->AddMessageHandler(
      std::make_unique<settings::MetricsReportingHandler>());
  web_ui->AddMessageHandler(std::make_unique<BravePrivacyHandler>());
  web_ui->AddMessageHandler(std::make_unique<DefaultBraveShieldsHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveDefaultExtensionsHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveAppearanceHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveSyncHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveWalletHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveAdBlockHandler>());
#if BUILDFLAG(ENABLE_TOR)
  web_ui->AddMessageHandler(std::make_unique<BraveTorHandler>());
#endif
#if BUILDFLAG(ENABLE_EXTENSIONS)
  web_ui->AddMessageHandler(
      std::make_unique<BraveTorSnowflakeExtensionHandler>());
  if (base::FeatureList::IsEnabled(kExtensionsManifestV2)) {
    web_ui->AddMessageHandler(
        std::make_unique<BraveExtensionsManifestV2Handler>());
  }
#endif
#if BUILDFLAG(ENABLE_PIN_SHORTCUT)
  web_ui->AddMessageHandler(std::make_unique<PinShortcutHandler>());
#endif
}

BraveSettingsUI::~BraveSettingsUI() = default;

// static
void BraveSettingsUI::AddResources(content::WebUIDataSource *html_source,
                                   Profile *profile) {
  for (size_t i = 0; i < kBraveSettingsResourcesSize; ++i) {
    html_source->AddResourcePath(kBraveSettingsResources[i].path,
                                 kBraveSettingsResources[i].id);
  }

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  for (size_t i = 0; i < kCommandsGeneratedSize; ++i) {
    html_source->AddResourcePath(kCommandsGenerated[i].path,
                                 kCommandsGenerated[i].id);
  }
#endif

  html_source->AddBoolean("isSyncDisabled", !syncer::IsSyncAllowedByFlag());
  html_source->AddString(
      "braveProductVersion",
      version_info::GetBraveVersionWithoutChromiumMajorVersion());
  NavigationBarDataProvider::Initialize(html_source, profile);
  if (auto *service = ViewCounterServiceFactory::GetForProfile(profile)) {
    service->InitializeWebUIDataSource(html_source);
  }
  html_source->AddBoolean(
      "isIdleDetectionFeatureEnabled",
      base::FeatureList::IsEnabled(features::kIdleDetection));
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  html_source->AddBoolean("isBraveVPNEnabled",
                          brave_vpn::IsBraveVPNEnabled(profile));
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
  html_source->AddBoolean(
      "isSpeedreaderFeatureEnabled",
      base::FeatureList::IsEnabled(speedreader::kSpeedreaderFeature));
#endif
  html_source->AddBoolean(
      "isNativeBraveWalletFeatureEnabled",
      base::FeatureList::IsEnabled(
          brave_wallet::features::kNativeBraveWalletFeature));
  html_source->AddBoolean("isBraveWalletAllowed",
                          brave_wallet::IsAllowedForContext(profile));
  html_source->AddBoolean("isForgetFirstPartyStorageFeatureEnabled",
                          base::FeatureList::IsEnabled(
                              net::features::kBraveForgetFirstPartyStorage));
  html_source->AddBoolean("isBraveRewardsSupported",
                          brave_rewards::IsSupportedForProfile(profile));
  html_source->AddBoolean(
      "areShortcutsSupported",
      base::FeatureList::IsEnabled(commands::features::kBraveCommands));

  if (ShouldDisableCSPForTesting()) {
    html_source->DisableContentSecurityPolicy();
  }

  html_source->AddBoolean("shouldExposeElementsForTesting",
                          ShouldExposeElementsForTesting());

  html_source->AddBoolean("enable_extensions", BUILDFLAG(ENABLE_EXTENSIONS));

  html_source->AddBoolean("extensionsManifestV2Feature",
                          base::FeatureList::IsEnabled(kExtensionsManifestV2));
#if defined(TOOLKIT_VIEWS)
  html_source->AddBoolean(
      "verticalTabStripFeatureEnabled",
      base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs));
#endif
}

// static
bool &BraveSettingsUI::ShouldDisableCSPForTesting() {
  static bool disable_csp = false;
  return disable_csp;
}

// static
bool &BraveSettingsUI::ShouldExposeElementsForTesting() {
  static bool expose_elements = false;
  return expose_elements;
}

void BraveSettingsUI::BindInterface(
    mojo::PendingReceiver<commands::mojom::CommandsService> pending_receiver) {
  commands::AcceleratorServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext())
      ->BindInterface(std::move(pending_receiver));
}
