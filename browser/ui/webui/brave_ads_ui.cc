/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_ads_ui.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

BraveAdsUI::BraveAdsUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name, kAdsJS,
        IDR_BRAVE_ADS_JS, IDR_BRAVE_ADS_HTML) {
  Profile* profile = Profile::FromWebUI(web_ui);
  PrefService* prefs = profile->GetPrefs();
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(prefs);
  pref_change_registrar_->Add(kAdsBlocked,
    base::Bind(&BraveAdsUI::OnPreferenceChanged, base::Unretained(this)));
}

BraveAdsUI::~BraveAdsUI() {
}

void BraveAdsUI::CustomizeWebUIProperties(content::RenderViewHost* render_view_host) {
  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* prefs = profile->GetPrefs();
  if (render_view_host) {
    render_view_host->SetWebUIProperty("adsBlockedStat", std::to_string(prefs->GetUint64(kAdsBlocked)));
    render_view_host->SetWebUIProperty("regionalAdBlockEnabled",
        std::to_string(
          g_brave_browser_process->ad_block_regional_service()->IsInitialized()));
    render_view_host->SetWebUIProperty("regionalAdBlockTitle",
        g_brave_browser_process->ad_block_regional_service()->GetTitle());
  }
}

void BraveAdsUI::RenderFrameCreated(content::RenderFrameHost* render_frame_host) {
  if (IsSafeToSetWebUIProperties()) {
    CustomizeWebUIProperties(render_frame_host->GetRenderViewHost());
  }
}

void BraveAdsUI::OnPreferenceChanged() {
  if (IsSafeToSetWebUIProperties()) {
    CustomizeWebUIProperties(GetRenderViewHost());
    web_ui()->CallJavascriptFunctionUnsafe("brave_adblock.statsUpdated");
  }
}
