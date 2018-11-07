/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/basic_ui.h"

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/bindings_policy.h"

content::WebUIDataSource* CreateBasicUIHTMLSource(Profile* profile,
                                                  const std::string& name,
                                                  const std::string& js_file,
                                                  int js_resource_id,
                                                  int html_resource_id) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(name);
  source->SetJsonPath("strings.js");
  source->SetDefaultResource(html_resource_id);
  source->AddResourcePath(js_file, js_resource_id);
  CustomizeWebUIHTMLSource(name, source);
  return source;
}

BasicUI::BasicUI(content::WebUI* web_ui,
    const std::string& name,
    const std::string& js_file,
    int js_resource_id,
    int html_resource_id)
    : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source = CreateBasicUIHTMLSource(profile, name,
      js_file, js_resource_id, html_resource_id);
  content::WebUIDataSource::Add(profile, source);
}

BasicUI::~BasicUI() {
}

content::RenderViewHost* BasicUI::GetRenderViewHost() {
  auto* web_contents = web_ui()->GetWebContents();
  if (web_contents) {
    return web_contents->GetRenderViewHost();
  }
  return nullptr;
}

bool BasicUI::IsSafeToSetWebUIProperties() const {
  // Allow `web_ui()->CanCallJavascript()` to be false.
  // Allow `web_ui()->CanCallJavascript()` to be true if `(web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI) != 0`
  // Disallow `web_ui()->CanCallJavascript()` to be true if `(web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI) == 0`
  DCHECK(!web_ui()->CanCallJavascript() || (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI));
  return web_ui()->CanCallJavascript() && (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI);
}
