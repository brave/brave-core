/* Copyright (c) 2019 The Brave Authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_md_settings_ui.h"

#include <string>
#include "base/command_line.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/browser/resources/settings/grit/brave_settings_resources.h"
#include "brave/browser/resources/settings/grit/brave_settings_resources_map.h"
#include "brave/browser/ui/webui/settings/brave_default_extensions_handler.h"
#include "brave/browser/ui/webui/settings/brave_privacy_handler.h"
#include "brave/browser/ui/webui/settings/default_brave_shields_handler.h"
#include "brave/browser/ui/webui/settings/brave_reset_rewards_settings_handler.h"  // NOLINT
#include "brave/common/brave_switches.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/settings/metrics_reporting_handler.h"
#include "content/public/browser/web_ui_data_source.h"

#if defined(OS_MACOSX)
#include "brave/browser/ui/webui/settings/brave_relaunch_handler_mac.h"
#endif

BraveMdSettingsUI::BraveMdSettingsUI(content::WebUI* web_ui,
                                     const std::string& host)
    : MdSettingsUI(web_ui) {
  web_ui->AddMessageHandler(
    std::make_unique<settings::MetricsReportingHandler>());
  web_ui->AddMessageHandler(std::make_unique<BravePrivacyHandler>());
  web_ui->AddMessageHandler(std::make_unique<DefaultBraveShieldsHandler>());
  web_ui->AddMessageHandler(std::make_unique<BraveDefaultExtensionsHandler>());
#if defined(OS_MACOSX)
  // Use sparkle's relaunch api for browser relaunch on update.
  web_ui->AddMessageHandler(std::make_unique<BraveRelaunchHandler>());
#endif
  web_ui->AddMessageHandler(
      std::make_unique<settings::BraveResetRewardsSettingsHandler>(
        Profile::FromWebUI(web_ui)));
}

BraveMdSettingsUI::~BraveMdSettingsUI() {
}

// static
void BraveMdSettingsUI::AddResources(content::WebUIDataSource* html_source,
                                Profile* profile) {
  for (size_t i = 0; i < kBraveSettingsResourcesSize; ++i) {
    html_source->AddResourcePath(kBraveSettingsResources[i].name,
                                 kBraveSettingsResources[i].value);
  }

  html_source->AddBoolean("isPdfjsDisabled",
                          extensions::BraveComponentLoader::IsPdfjsDisabled());
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  html_source->AddBoolean("isSyncDisabled",
                          command_line.HasSwitch(switches::kDisableBraveSync));
}
