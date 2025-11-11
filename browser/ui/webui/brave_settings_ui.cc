/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_settings_ui.h"

#include <memory>
#include <string>
#include <utility>

#include "base/compiler_specific.h"
#include "base/feature_list.h"
#include "brave/browser/brave_origin/brave_origin_service_factory.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/email_aliases/email_aliases_service_factory.h"
#include "brave/browser/resources/settings/grit/brave_settings_resources.h"
#include "brave/browser/resources/settings/grit/brave_settings_resources_map.h"
#include "brave/browser/shell_integrations/buildflags/buildflags.h"
#include "brave/browser/ui/commands/accelerator_service_factory.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "brave/browser/ui/webui/settings/brave_account_settings_handler.h"
#include "brave/browser/ui/webui/settings/brave_adblock_handler.h"
#include "brave/browser/ui/webui/settings/brave_appearance_handler.h"
#include "brave/browser/ui/webui/settings/brave_default_extensions_handler.h"
#include "brave/browser/ui/webui/settings/brave_privacy_handler.h"
#include "brave/browser/ui/webui/settings/brave_sync_handler.h"
#include "brave/browser/ui/webui/settings/brave_wallet_handler.h"
#include "brave/browser/ui/webui/settings/default_brave_shields_handler.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/brave_origin/brave_origin_handler.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/commander/common/features.h"
#include "brave/components/commands/common/commands.mojom.h"
#include "brave/components/commands/common/features.h"
#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "brave/components/email_aliases/features.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/playlist/core/common/features.h"
#include "brave/components/playlist/core/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/version_info/version_info.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/regional_capabilities/regional_capabilities_service_factory.h"
#include "chrome/browser/ui/webui/settings/metrics_reporting_handler.h"
#include "components/regional_capabilities/regional_capabilities_country_id.h"
#include "components/regional_capabilities/regional_capabilities_service.h"
#include "components/sync/base/command_line_switches.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/content_features.h"
#include "extensions/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/base/features.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_settings_helper.h"
#include "brave/browser/ui/webui/settings/brave_settings_leo_assistant_handler.h"
#include "brave/components/ai_chat/core/browser/customization_settings_handler.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#endif

#if BUILDFLAG(ENABLE_PIN_SHORTCUT)
#include "brave/browser/ui/webui/settings/pin_shortcut_handler.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/browser/brave_vpn/vpn_utils.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#if BUILDFLAG(IS_WIN)
#include "brave/browser/ui/webui/settings/brave_vpn/brave_vpn_handler.h"
#endif
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/ui/webui/settings/brave_tor_handler.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/extensions/manifest_v2/features.h"
#include "brave/browser/ui/webui/settings/brave_extensions_manifest_v2_handler.h"
#include "brave/browser/ui/webui/settings/brave_tor_snowflake_extension_handler.h"
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/core/browser/containers_settings_handler.h"
#include "brave/components/containers/core/common/features.h"
#endif

namespace {

bool IsLocaleJapan(Profile* profile) {
  if (auto* regional_capabilities = regional_capabilities::
          RegionalCapabilitiesServiceFactory::GetForProfile(profile)) {
    return regional_capabilities->GetCountryId() ==
           regional_capabilities::CountryIdHolder(
               country_codes::CountryId("JP"));
  }
  return false;
}

}  // namespace

BraveSettingsUI::BraveSettingsUI(content::WebUI* web_ui) : SettingsUI(web_ui) {
  web_ui->AddMessageHandler(
      std::make_unique<settings::MetricsReportingHandler>());
  web_ui->AddMessageHandler(std::make_unique<BravePrivacyHandler>());
  web_ui->AddMessageHandler(std::make_unique<DefaultBraveShieldsHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveDefaultExtensionsHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveAppearanceHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveSyncHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveWalletHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveAdBlockHandler>());
#if BUILDFLAG(ENABLE_AI_CHAT)
  web_ui->AddMessageHandler(
      std::make_unique<settings::BraveLeoAssistantHandler>());
#endif

#if BUILDFLAG(ENABLE_TOR)
  web_ui->AddMessageHandler(std::make_unique<BraveTorHandler>());
#endif
#if BUILDFLAG(ENABLE_EXTENSIONS)
  web_ui->AddMessageHandler(
      std::make_unique<BraveTorSnowflakeExtensionHandler>());
  if (base::FeatureList::IsEnabled(
          extensions_mv2::features::kExtensionsManifestV2)) {
    web_ui->AddMessageHandler(
        std::make_unique<BraveExtensionsManifestV2Handler>());
  }
#endif
#if BUILDFLAG(ENABLE_PIN_SHORTCUT)
  web_ui->AddMessageHandler(std::make_unique<PinShortcutHandler>());
#endif
#if BUILDFLAG(IS_WIN) && BUILDFLAG(ENABLE_BRAVE_VPN)
  if (brave_vpn::IsBraveVPNEnabled(Profile::FromWebUI(web_ui))) {
    web_ui->AddMessageHandler(
        std::make_unique<BraveVpnHandler>(Profile::FromWebUI(web_ui)));
  }
#endif
}

BraveSettingsUI::~BraveSettingsUI() = default;

// static
void BraveSettingsUI::AddResources(content::WebUIDataSource* html_source,
                                   Profile* profile) {
  html_source->AddResourcePaths(kBraveSettingsResources);

  html_source->AddBoolean("isSyncDisabled", !syncer::IsSyncAllowedByFlag());
  html_source->AddString(
      "braveProductVersion",
      version_info::GetBraveVersionWithoutChromiumMajorVersion());
  NavigationBarDataProvider::Initialize(html_source, profile);
  html_source->AddBoolean(
      "isIdleDetectionFeatureEnabled",
      base::FeatureList::IsEnabled(features::kIdleDetection));
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  html_source->AddBoolean("isBraveVPNEnabled",
                          brave_vpn::IsBraveVPNEnabled(profile));
#if BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_BRAVE_VPN_WIREGUARD)
  html_source->AddBoolean(
      "isBraveVPNWireguardEnabledOnMac",
      base::FeatureList::IsEnabled(
          brave_vpn::features::kBraveVPNEnableWireguardForOSX));
#endif  // BUILDFLAG(IS_MAC) && BUILDFLAG(ENABLE_BRAVE_VPN_WIREGUARD)
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)
#if BUILDFLAG(ENABLE_SPEEDREADER)
  html_source->AddBoolean(
      "isSpeedreaderAllowed",
      base::FeatureList::IsEnabled(
          speedreader::features::kSpeedreaderFeature) &&
          (profile->GetPrefs()->GetBoolean(speedreader::kSpeedreaderEnabled) ||
           !profile->GetPrefs()->IsManagedPreference(
               speedreader::kSpeedreaderEnabled)));
#endif
  html_source->AddBoolean(
      "isNativeBraveWalletFeatureEnabled",
      base::FeatureList::IsEnabled(
          brave_wallet::features::kNativeBraveWalletFeature));
  html_source->AddBoolean("isCardanoDappSupportFeatureEnabled",
                          brave_wallet::IsCardanoDAppSupportEnabled());
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

  html_source->AddBoolean("shouldExposeElementsForTesting",
                          ShouldExposeElementsForTesting());

  html_source->AddBoolean("enable_extensions", BUILDFLAG(ENABLE_EXTENSIONS));

  html_source->AddBoolean("extensionsManifestV2Feature",
                          base::FeatureList::IsEnabled(
                              extensions_mv2::features::kExtensionsManifestV2));

#if BUILDFLAG(ENABLE_AI_CHAT)
  html_source->AddBoolean("isLeoAssistantAllowed",
                          ai_chat::IsAIChatEnabled(profile->GetPrefs()));
  html_source->AddBoolean("isLeoAssistantHistoryAllowed",
                          ai_chat::features::IsAIChatHistoryEnabled());
#endif

  html_source->AddBoolean("isSurveyPanelistAllowed",
                          base::FeatureList::IsEnabled(
                              ntp_background_images::features::
                                  kBraveNTPBrandedWallpaperSurveyPanelist));
  html_source->AddBoolean(
      "isPlaylistAllowed",
      base::FeatureList::IsEnabled(playlist::features::kPlaylist) &&
          profile->GetPrefs()->GetBoolean(playlist::kPlaylistEnabledPref));

  html_source->AddBoolean(
      "showCommandsInOmnibox",
      base::FeatureList::IsEnabled(features::kBraveCommandsInOmnibox));
  html_source->AddBoolean(
      "isSharedPinnedTabsEnabled",
      base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs));
  html_source->AddBoolean(
      "isEmailAliasesEnabled",
      base::FeatureList::IsEnabled(email_aliases::features::kEmailAliases));
#if BUILDFLAG(ENABLE_CONTAINERS)
  html_source->AddBoolean(
      "isContainersEnabled",
      base::FeatureList::IsEnabled(containers::features::kContainers));
#endif
  html_source->AddBoolean("isBraveAccountEnabled",
                          brave_account::features::IsBraveAccountEnabled());
  html_source->AddBoolean("isOriginAllowed",
                          brave_origin::IsBraveOriginEnabled());
  html_source->AddBoolean(
      "isTreeTabsFlagEnabled",
      base::FeatureList::IsEnabled(tabs::features::kBraveTreeTab));
  html_source->AddString("braveSearchEngineName",
                         TemplateURLPrepopulateData::brave_search.name);
  html_source->AddBoolean("isLocaleJapan", IsLocaleJapan(profile));
  html_source->AddBoolean("isHideVerticalTabCompletelyFlagEnabled",
                          base::FeatureList::IsEnabled(
                              tabs::features::kBraveVerticalTabHideCompletely));
}

// static
bool& BraveSettingsUI::ShouldExposeElementsForTesting() {
  static bool expose_elements = false;
  return expose_elements;
}

void BraveSettingsUI::BindInterface(
    mojo::PendingReceiver<commands::mojom::CommandsService> pending_receiver) {
  commands::AcceleratorServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext())
      ->BindInterface(std::move(pending_receiver));
}

#if BUILDFLAG(ENABLE_AI_CHAT)
void BraveSettingsUI::BindInterface(
    mojo::PendingReceiver<ai_chat::mojom::AIChatSettingsHelper>
        pending_receiver) {
  auto helper = std::make_unique<ai_chat::AIChatSettingsHelper>(
      web_ui()->GetWebContents()->GetBrowserContext());
  mojo::MakeSelfOwnedReceiver(std::move(helper), std::move(pending_receiver));
}

void BraveSettingsUI::BindInterface(
    mojo::PendingReceiver<ai_chat::mojom::CustomizationSettingsHandler>
        pending_receiver) {
  auto handler = std::make_unique<ai_chat::CustomizationSettingsHandler>(
      user_prefs::UserPrefs::Get(
          web_ui()->GetWebContents()->GetBrowserContext()));
  mojo::MakeSelfOwnedReceiver(std::move(handler), std::move(pending_receiver));
}
#endif

void BraveSettingsUI::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::BraveAccountSettingsHandler>
        pending_receiver) {
  brave_account_settings_handler_ =
      std::make_unique<brave_account::BraveAccountSettingsHandler>(
          std::move(pending_receiver), web_ui());
}

#if BUILDFLAG(ENABLE_CONTAINERS)
void BraveSettingsUI::BindInterface(
    mojo::PendingReceiver<containers::mojom::ContainersSettingsHandler>
        pending_receiver) {
  if (!base::FeatureList::IsEnabled(containers::features::kContainers)) {
    return;
  }
  auto handler = std::make_unique<containers::ContainersSettingsHandler>(
      user_prefs::UserPrefs::Get(
          web_ui()->GetWebContents()->GetBrowserContext()));
  mojo::MakeSelfOwnedReceiver(std::move(handler), std::move(pending_receiver));
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

void BraveSettingsUI::BindInterface(
    mojo::PendingReceiver<email_aliases::mojom::EmailAliasesService> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  email_aliases::EmailAliasesServiceFactory::BindForProfile(
      profile, std::move(receiver));
}

void BraveSettingsUI::BindInterface(
    mojo::PendingReceiver<brave_origin::mojom::BraveOriginSettingsHandler>
        receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  auto* brave_origin_service =
      brave_origin::BraveOriginServiceFactory::GetForProfile(profile);
  // Service may be null for Guest profiles
  if (brave_origin_service) {
    auto handler =
        std::make_unique<brave_origin::BraveOriginSettingsHandlerImpl>(
            brave_origin_service);
    mojo::MakeSelfOwnedReceiver(std::move(handler), std::move(receiver));
  }
}
