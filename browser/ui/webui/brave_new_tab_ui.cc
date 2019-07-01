// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_ui.h"

#include <string>

#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/browser/ui/webui/brave_new_tab_message_handler.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_new_tab/resources/grit/brave_new_tab_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_ui_data_source.h"

BraveNewTabUI::BraveNewTabUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name, kBraveNewTabGenerated,
        kBraveNewTabGeneratedSize, IDR_BRAVE_NEW_TAB_HTML) {
  web_ui->AddMessageHandler(std::make_unique<BraveNewTabMessageHandler>(this));
}

BraveNewTabUI::~BraveNewTabUI() {
}

void BraveNewTabUI::UpdateWebUIProperties() {
  if (IsSafeToSetWebUIProperties()) {
    // TODO(petemill): move all this data to set on loadTimeData
    // on the DataSource via the MessageHandler
    auto* render_view_host = GetRenderViewHost();
    SetStatsWebUIProperties(render_view_host);
    SetPrivateWebUIProperties(render_view_host);
    SetPreferencesWebUIProperties(render_view_host);
  }
}

void BraveNewTabUI::SetStatsWebUIProperties(
  content::RenderViewHost* render_view_host) {
  DCHECK(IsSafeToSetWebUIProperties());
  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* prefs = profile->GetPrefs();
  if (render_view_host) {
    render_view_host->SetWebUIProperty(
        "adsBlockedStat",
        std::to_string(prefs->GetUint64(kAdsBlocked)));
    render_view_host->SetWebUIProperty(
        "trackersBlockedStat",
        std::to_string(prefs->GetUint64(kTrackersBlocked)));
    render_view_host->SetWebUIProperty(
        "javascriptBlockedStat",
        std::to_string(prefs->GetUint64(kJavascriptBlocked)));
    render_view_host->SetWebUIProperty(
        "httpsUpgradesStat",
        std::to_string(prefs->GetUint64(kHttpsUpgrades)));
    render_view_host->SetWebUIProperty(
        "fingerprintingBlockedStat",
        std::to_string(prefs->GetUint64(kFingerprintingBlocked)));
    render_view_host->SetWebUIProperty(
        "useAlternativePrivateSearchEngine",
        prefs->GetBoolean(kUseAlternativeSearchEngineProvider) ? "true"
                                                               : "false");
    render_view_host->SetWebUIProperty(
        "isTor", profile->IsTorProfile() ? "true" : "false");
    render_view_host->SetWebUIProperty(
        "isQwant", brave::IsRegionForQwant(profile) ? "true" : "false");
  }
}

void BraveNewTabUI::SetPrivateWebUIProperties(
  content::RenderViewHost* render_view_host) {
  DCHECK(IsSafeToSetWebUIProperties());
  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* prefs = profile->GetPrefs();
  if (render_view_host) {
    render_view_host->SetWebUIProperty(
        "useAlternativePrivateSearchEngine",
        prefs->GetBoolean(kUseAlternativeSearchEngineProvider) ? "true"
                                                               : "false");
    render_view_host->SetWebUIProperty(
        "isTor", profile->IsTorProfile() ? "true" : "false");
    render_view_host->SetWebUIProperty(
        "isQwant", brave::IsRegionForQwant(profile) ? "true" : "false");
  }
}

void BraveNewTabUI::SetPreferencesWebUIProperties(
  content::RenderViewHost* render_view_host) {
  DCHECK(IsSafeToSetWebUIProperties());
  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* prefs = profile->GetPrefs();
  if (render_view_host) {
    render_view_host->SetWebUIProperty(
        "showBackgroundImage",
        prefs->GetBoolean(kNewTabPageShowBackgroundImage) ? "true"
                                                          : "false");
  }
}


void BraveNewTabUI::OnPreferencesChanged() {
  if (IsSafeToSetWebUIProperties()) {
    SetPreferencesWebUIProperties(GetRenderViewHost());
  }
}

void BraveNewTabUI::OnPrivatePropertiesChanged() {
  if (IsSafeToSetWebUIProperties()) {
    SetPrivateWebUIProperties(GetRenderViewHost());
  }
}

void BraveNewTabUI::OnStatsChanged() {
  if (IsSafeToSetWebUIProperties()) {
    SetStatsWebUIProperties(GetRenderViewHost());
  }
}
