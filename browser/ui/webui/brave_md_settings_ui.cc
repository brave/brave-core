/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_md_settings_ui.h"

#include "brave/browser/resources/grit/brave_settings_resources.h"
#include "brave/browser/resources/grit/brave_settings_resources_map.h"
#include "brave/browser/ui/webui/settings/brave_privacy_handler.h"
#include "brave/browser/ui/webui/settings/default_brave_shields_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/settings/metrics_reporting_handler.h"
#include "content/public/browser/web_ui_data_source.h"

BraveMdSettingsUI::BraveMdSettingsUI(content::WebUI* web_ui,
                                     const std::string& host)
    : MdSettingsUI(web_ui) {
  web_ui->AddMessageHandler(std::make_unique<settings::MetricsReportingHandler>());
  web_ui->AddMessageHandler(std::make_unique<BravePrivacyHandler>());
  web_ui->AddMessageHandler(std::make_unique<DefaultBraveShieldsHandler>());
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
}
