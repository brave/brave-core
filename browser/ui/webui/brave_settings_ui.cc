/* Copyright (c) 2019 The Brave Authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_settings_ui.h"

#include <string>

#include "base/feature_list.h"
#include "brave/browser/ntp_background_images/view_counter_service_factory.h"
#include "brave/browser/resources/settings/grit/brave_settings_resources.h"
#include "brave/browser/resources/settings/grit/brave_settings_resources_map.h"
#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "brave/browser/ui/webui/settings/brave_appearance_handler.h"
#include "brave/browser/ui/webui/settings/brave_default_extensions_handler.h"
#include "brave/browser/ui/webui/settings/brave_privacy_handler.h"
#include "brave/browser/ui/webui/settings/brave_sync_handler.h"
#include "brave/browser/ui/webui/settings/brave_wallet_handler.h"
#include "brave/browser/ui/webui/settings/default_brave_shields_handler.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "brave/components/speedreader/buildflags.h"
#include "brave/components/version_info/version_info.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/settings/metrics_reporting_handler.h"
#include "components/sync/base/command_line_switches.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/content_features.h"

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/features.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#endif

#if defined(OS_WIN)
#include "brave/browser/ui/webui/settings/ms_edge_protocol_message_handler.h"
#endif

using ntp_background_images::ViewCounterServiceFactory;

BraveSettingsUI::BraveSettingsUI(content::WebUI* web_ui,
                                 const std::string& host)
    : SettingsUI(web_ui) {
  web_ui->AddMessageHandler(
      std::make_unique<settings::MetricsReportingHandler>());
  web_ui->AddMessageHandler(std::make_unique<BravePrivacyHandler>());
  web_ui->AddMessageHandler(std::make_unique<DefaultBraveShieldsHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveDefaultExtensionsHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveAppearanceHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveSyncHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveWalletHandler>());
#if defined(OS_WIN)
  if (MSEdgeProtocolMessageHandler::CanSetDefaultMSEdgeProtocolHandler())
    web_ui->AddMessageHandler(std::make_unique<MSEdgeProtocolMessageHandler>());
#endif
}

BraveSettingsUI::~BraveSettingsUI() {}

// static
void BraveSettingsUI::AddResources(content::WebUIDataSource* html_source,
                                   Profile* profile) {
  for (size_t i = 0; i < kBraveSettingsResourcesSize; ++i) {
    html_source->AddResourcePath(kBraveSettingsResources[i].path,
                                 kBraveSettingsResources[i].id);
  }

  html_source->AddBoolean("isSyncDisabled", !syncer::IsSyncAllowedByFlag());
  html_source->AddString(
      "braveProductVersion",
      version_info::GetBraveVersionWithoutChromiumMajorVersion());
  NavigationBarDataProvider::Initialize(html_source);
  if (auto* service = ViewCounterServiceFactory::GetForProfile(profile))
    service->InitializeWebUIDataSource(html_source);
  html_source->AddBoolean(
      "isIdleDetectionFeatureEnabled",
      base::FeatureList::IsEnabled(features::kIdleDetection));
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  html_source->AddBoolean("isBraveVPNEnabled", brave_vpn::IsBraveVPNEnabled());
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
  // TODO(keur): Remove this when Speedreader feature enabled by default.
  html_source->AddBoolean(
      "isSpeedreaderFeatureEnabled",
      base::FeatureList::IsEnabled(speedreader::kSpeedreaderFeature));
#endif
  html_source->AddBoolean(
      "isNativeBraveWalletFeatureEnabled",
      base::FeatureList::IsEnabled(
          brave_wallet::features::kNativeBraveWalletFeature));
#if defined(OS_WIN)
  html_source->AddBoolean(
      "canSetDefaultMSEdgeProtocolHandler",
      MSEdgeProtocolMessageHandler::CanSetDefaultMSEdgeProtocolHandler());
#endif
}
