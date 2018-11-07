/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_new_tab_ui.h"

#include "brave/browser/search_engine_provider_util.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace {
class NewTabDOMHandler : public content::WebUIMessageHandler {
 public:
  NewTabDOMHandler() = default;
  ~NewTabDOMHandler() override = default;

 private:
  // WebUIMessageHandler implementation.
  void RegisterMessages() override {
    web_ui()->RegisterMessageCallback(
        "toggleAlternativePrivateSearchEngine",
        base::BindRepeating(
            &NewTabDOMHandler::HandleToggleAlternativeSearchEngineProvider,
            base::Unretained(this)));
  }

  void HandleToggleAlternativeSearchEngineProvider(
      const base::ListValue* args) {
    brave::ToggleUseAlternativeSearchEngineProvider(
        Profile::FromWebUI(web_ui()));
  }

  DISALLOW_COPY_AND_ASSIGN(NewTabDOMHandler);
};

bool IsRegionForQwant(Profile* profile) {
  return TemplateURLPrepopulateData::GetPrepopulatedDefaultSearch(
      profile->GetPrefs())->prepopulate_id ==
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT;
}

}  // namespace

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
  pref_change_registrar_->Add(kUseAlternativeSearchEngineProvider,
    base::Bind(&BraveNewTabUI::OnPreferenceChanged, base::Unretained(this)));
  pref_change_registrar_->Add(kAlternativeSearchEngineProviderInTor,
    base::Bind(&BraveNewTabUI::OnPreferenceChanged, base::Unretained(this)));

  web_ui->AddMessageHandler(std::make_unique<NewTabDOMHandler>());
}

BraveNewTabUI::~BraveNewTabUI() {
}

void BraveNewTabUI::CustomizeNewTabWebUIProperties(content::RenderViewHost* render_view_host) {
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
        "isQwant", IsRegionForQwant(profile) ? "true" : "false");
  }
}

void BraveNewTabUI::UpdateWebUIProperties() {
  if (IsSafeToSetWebUIProperties()) {
    CustomizeNewTabWebUIProperties(GetRenderViewHost());
    web_ui()->CallJavascriptFunctionUnsafe("brave_new_tab.statsUpdated");
  }
}

void BraveNewTabUI::OnPreferenceChanged() {
  UpdateWebUIProperties();
}
