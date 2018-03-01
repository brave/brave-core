/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/basic_ui.h"

#include "brave/browser/ui/webui/new_tab_html_source.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/bindings_policy.h"
#include "ui/base/resource/resource_bundle.h"


using content::WebContents;
using content::WebUIMessageHandler;

namespace {

content::WebUIDataSource* CreateBasicUIHTMLSource(Profile* profile,
                                                  const std::string& name,
                                                  const std::string& js_file,
                                                  int js_resource_id,
                                                  int html_resource_id) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(name);

  CustomizeNewTabHTMLSource(source);

  source->SetJsonPath("strings.js");
  source->SetDefaultResource(html_resource_id);
  source->AddResourcePath(js_file, js_resource_id);
  return source;
}

// The handler for Javascript messages for Brave about: pages
class BasicDOMHandler : public WebUIMessageHandler {
 public:
  BasicDOMHandler() {
  }
  ~BasicDOMHandler() override {}

  void Init();

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BasicDOMHandler);
};

void BasicDOMHandler::RegisterMessages() {
}

void BasicDOMHandler::Init() {
}

}  // namespace

BasicUI::BasicUI(content::WebUI* web_ui,
    const std::string& name,
    const std::string& js_file,
    int js_resource_id,
    int html_resource_id)
    : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  auto handler_owner = base::MakeUnique<BasicDOMHandler>();
  BasicDOMHandler* handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
  content::WebUIDataSource::Add(profile,
    CreateBasicUIHTMLSource(profile, name,
      js_file, js_resource_id, html_resource_id));
  PrefService* prefs = profile->GetPrefs();
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(prefs);
  pref_change_registrar_->Add(kAdsBlocked,
    base::Bind(&BasicUI::OnPreferenceChanged, base::Unretained(this)));
  pref_change_registrar_->Add(kTrackersBlocked,
    base::Bind(&BasicUI::OnPreferenceChanged, base::Unretained(this)));
  pref_change_registrar_->Add(kHttpsUpgrades,
    base::Bind(&BasicUI::OnPreferenceChanged, base::Unretained(this)));
}

BasicUI::~BasicUI() {
  pref_change_registrar_.reset();
}

void BasicUI::RenderFrameCreated(content::RenderFrameHost* render_frame_host) {
  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    CustomizeNewTabWebUIProperties(web_ui());
  }
}

void BasicUI::OnPreferenceChanged() {
  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    CustomizeNewTabWebUIProperties(web_ui());
    web_ui()->CallJavascriptFunctionUnsafe("brave_new_tab.statsUpdated");
  }
}

