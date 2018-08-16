/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/basic_ui.h"

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

using content::WebUIMessageHandler;

namespace {

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
  auto handler_owner = std::make_unique<BasicDOMHandler>();
  BasicDOMHandler* handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
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
