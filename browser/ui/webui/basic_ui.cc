/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/basic_ui.h"

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/bindings_policy.h"
// TODO(petemill): The following is being included purely to get the generated
// GritResourceMap definition. Replace with a better solution.
#if !defined(OS_ANDROID)
#include "brave/components/brave_new_tab/resources/grit/brave_new_tab_generated_map.h"
#else
#include "components/brave_rewards/settings/resources/grit/brave_rewards_settings_generated_map.h"
#endif
#include "ui/resources/grit/webui_resources_map.h"

content::WebUIDataSource* CreateBasicUIHTMLSource(
    Profile* profile,
    const std::string& name,
    const GzippedGritResourceMap* resource_map,
    size_t resource_map_size,
    int html_resource_id) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(name);
  source->SetJsonPath("strings.js");
  source->SetDefaultResource(html_resource_id);
  // Add generated resource paths
  for (size_t i = 0; i < resource_map_size; ++i) {
    source->AddResourcePath(resource_map[i].name,  resource_map[i].value);
  }
  CustomizeWebUIHTMLSource(name, source);
  return source;
}

// This is used to know proper timing for setting webui properties.
// So far, content::WebUIController::RenderFrameCreated() is used.
// However, it doesn't get called sometimes when reloading, or called when
// RenderFrameHost is not prepared during renderer process is in changing.
class BasicUI::BasicUIWebContentsObserver
    : public content::WebContentsObserver {
 public:
  BasicUIWebContentsObserver(BasicUI* host, content::WebContents* web_contents)
      : WebContentsObserver(web_contents),
        host_(host) {
  }
  ~BasicUIWebContentsObserver() override {}

  // content::WebContentsObserver overrides:
  void RenderViewReady() override {
    host_->UpdateWebUIProperties();
  }

 private:
  BasicUI* host_;

  DISALLOW_COPY_AND_ASSIGN(BasicUIWebContentsObserver);
};

BasicUI::BasicUI(content::WebUI* web_ui,
                 const std::string& name,
                 const GzippedGritResourceMap* resource_map,
                 size_t resource_map_size,
                 int html_resource_id)
    : WebUIController(web_ui) {
  observer_.reset(
      new BasicUIWebContentsObserver(this, web_ui->GetWebContents()));
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source = CreateBasicUIHTMLSource(profile, name,
      resource_map, resource_map_size, html_resource_id);
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
  // Allow `web_ui()->CanCallJavascript()` to be true if
  // `(web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI) != 0`
  // Disallow `web_ui()->CanCallJavascript()` to be true if
  // `(web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI) == 0`
  DCHECK(!web_ui()->CanCallJavascript() ||
         (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI));
  return web_ui()->CanCallJavascript() &&
         (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI);
}
