/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/basic_ui.h"

#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/base/resource/resource_bundle.h"

using content::WebContents;
using content::WebUIMessageHandler;

namespace {

content::WebUIDataSource* CreateBasicUIHTMLSource(const std::string& name) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(kPaymentsHost);

  if (name == kPaymentsHost) {
    source->AddResourcePath(kPaymentsJS, IDR_BRAVE_PAYMENTS_JS);
    source->SetDefaultResource(IDR_BRAVE_PAYMENTS_HTML);
  }

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

BasicUI::BasicUI(content::WebUI* web_ui, const std::string& name)
    : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);

  auto handler_owner = base::MakeUnique<BasicDOMHandler>();
  BasicDOMHandler* handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
  content::WebUIDataSource::Add(profile, CreateBasicUIHTMLSource(name));
}
