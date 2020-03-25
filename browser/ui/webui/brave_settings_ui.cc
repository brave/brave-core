/* Copyright (c) 2019 The Brave Authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_settings_ui.h"

#include <string>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/browser/resources/settings/grit/brave_settings_resources.h"
#include "brave/browser/resources/settings/grit/brave_settings_resources_map.h"
#include "brave/browser/sparkle_buildflags.h"
#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "brave/browser/ui/webui/settings/brave_appearance_handler.h"
#include "brave/browser/ui/webui/settings/brave_default_extensions_handler.h"
#include "brave/browser/ui/webui/settings/brave_privacy_handler.h"
#include "brave/browser/ui/webui/settings/default_brave_shields_handler.h"
#include "brave/browser/version_info.h"
#include "brave/components/brave_sync/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/settings/metrics_reporting_handler.h"
#include "content/public/browser/web_ui_data_source.h"

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
#include "components/sync/driver/sync_driver_switches.h"
#endif

#if BUILDFLAG(ENABLE_SPARKLE)
#include "brave/browser/ui/webui/settings/brave_relaunch_handler_mac.h"
#endif

BraveSettingsUI::BraveSettingsUI(content::WebUI* web_ui,
                                 const std::string& host)
    : SettingsUI(web_ui) {
  web_ui->AddMessageHandler(
    std::make_unique<settings::MetricsReportingHandler>());
  web_ui->AddMessageHandler(std::make_unique<BravePrivacyHandler>());
  web_ui->AddMessageHandler(std::make_unique<DefaultBraveShieldsHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveDefaultExtensionsHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveAppearanceHandler>());
#if BUILDFLAG(ENABLE_SPARKLE)
  // Use sparkle's relaunch api for browser relaunch on update.
  web_ui->AddMessageHandler(std::make_unique<BraveRelaunchHandler>());
#endif
}

BraveSettingsUI::~BraveSettingsUI() {
}

// static
void BraveSettingsUI::AddResources(content::WebUIDataSource* html_source,
                                   Profile* profile) {
  for (size_t i = 0; i < kBraveSettingsResourcesSize; ++i) {
    html_source->AddResourcePath(kBraveSettingsResources[i].name,
                                 kBraveSettingsResources[i].value);
  }

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
  html_source->AddBoolean("isSyncDisabled",
                          !switches::IsSyncAllowedByFlag());
#else
  html_source->AddBoolean("isSyncDisabled", true);
#endif
  html_source->AddString("braveProductVersion",
    version_info::GetBraveVersionWithoutChromiumMajorVersion());
  NavigationBarDataProvider::Initialize(html_source);
  g_brave_browser_process->ntp_background_images_service()->
      InitializeWebUIDataSource(html_source);
}
