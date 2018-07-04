/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_new_tab_ui.h"

#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/bindings_policy.h"

BraveNewTabUI::BraveNewTabUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name, kBraveNewTabJS,
        IDR_BRAVE_NEW_TAB_JS, IDR_BRAVE_NEW_TAB_HTML) {
  Profile* profile = Profile::FromWebUI(web_ui);
  PrefService* prefs = profile->GetPrefs();
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(prefs);
  pref_change_registrar_->Add(kAdsBlocked,
    base::Bind(&BraveNewTabUI::OnPreferenceChanged, base::Unretained(this)));
  pref_change_registrar_->Add(kTrackersBlocked,
    base::Bind(&BraveNewTabUI::OnPreferenceChanged, base::Unretained(this)));
  pref_change_registrar_->Add(kHttpsUpgrades,
    base::Bind(&BraveNewTabUI::OnPreferenceChanged, base::Unretained(this)));
}

BraveNewTabUI::~BraveNewTabUI() {
  pref_change_registrar_.reset();
}

void BraveNewTabUI::CustomizeNewTabWebUIProperties() {
  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* prefs = profile->GetPrefs();
  auto* web_contents = web_ui()->GetWebContents();
  if (web_contents) {
    auto* render_view_host = web_contents->GetRenderViewHost();
    if (render_view_host) {
      render_view_host->SetWebUIProperty("adsBlockedStat", std::to_string(prefs->GetUint64(kAdsBlocked)));
      render_view_host->SetWebUIProperty("trackersBlockedStat", std::to_string(prefs->GetUint64(kTrackersBlocked)));
      render_view_host->SetWebUIProperty("javascriptBlockedStat", std::to_string(prefs->GetUint64(kJavascriptBlocked)));
      render_view_host->SetWebUIProperty("javascriptBlockedStat", std::to_string(prefs->GetUint64(kJavascriptBlocked)));
      render_view_host->SetWebUIProperty("httpsUpgradesStat", std::to_string(prefs->GetUint64(kHttpsUpgrades)));
      render_view_host->SetWebUIProperty("fingerprintingBlockedStat", std::to_string(prefs->GetUint64(kFingerprintingBlocked)));
    }
  }
}

void BraveNewTabUI::RenderFrameCreated(content::RenderFrameHost* render_frame_host) {
  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    CustomizeNewTabWebUIProperties();
  }
}

void BraveNewTabUI::OnPreferenceChanged() {
  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    CustomizeNewTabWebUIProperties();
    web_ui()->CallJavascriptFunctionUnsafe("brave_new_tab.statsUpdated");
  }
}
