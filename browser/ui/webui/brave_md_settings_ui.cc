/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_md_settings_ui.h"

#include "chrome/browser/ui/webui/settings/metrics_reporting_handler.h"

BraveMdSettingsUI::BraveMdSettingsUI(content::WebUI* web_ui,
                                     const std::string& host)
    : MdSettingsUI(web_ui) {
#if defined(BRAVE_CHROMIUM_BUILD) && defined(OFFICIAL_BUILD)
  web_ui->AddMessageHandler(std::make_unique<settings::MetricsReportingHandler>());
#endif
}

BraveMdSettingsUI::~BraveMdSettingsUI() {
}
