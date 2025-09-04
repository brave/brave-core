/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/ui/webui/new_tab_takeover_ui/new_tab_takeover_ui.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/values.h"
#include "brave/components/new_tab_takeover/mojom/new_tab_takeover.mojom.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_message_handler.h"
#include "url/gurl.h"

namespace {

// class NewTabTakeoverUIMessageHandler : public web::WebUIIOSMessageHandler {
//  public:
//   NewTabTakeoverUIMessageHandler() = default;

//   NewTabTakeoverUIMessageHandler(const NewTabTakeoverUIMessageHandler&) = delete;
//   NewTabTakeoverUIMessageHandler& operator=(const NewTabTakeoverUIMessageHandler&) =
//       delete;

//   ~NewTabTakeoverUIMessageHandler() override = default;

//  private:
//   // WebUIIOSMessageHandler:
//   void RegisterMessages() override;

//   void OnDialogCloseMessage(const base::Value::List& args);
// };

// void NewTabTakeoverUIMessageHandler::RegisterMessages() {
//   web_ui()->RegisterMessageCallback(
//       "dialogClose",
//       base::BindRepeating(&NewTabTakeoverUIMessageHandler::OnDialogCloseMessage,
//                           base::Unretained(this)));
// }

// void NewTabTakeoverUIMessageHandler::OnDialogCloseMessage(
//     const base::Value::List& args) {
//   web_ui()->GetWebState()->CloseWebState();
// }

ProfileIOS* GetProfile(web::WebUIIOS* web_ui) {
  ProfileIOS* const profile = ProfileIOS::FromWebUIIOS(web_ui);
  CHECK(profile);
  return profile;
}

void OverrideContentSecurityPolicy(web::WebUIDataSource* source) {
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome://resources 'self' 'wasm-unsafe-eval';");

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      base::StrCat({"frame-src ", kNTPNewTabTakeoverRichMediaUrl, ";"}));
}

web::WebUIIOSDataSource* CreateAndAddWebUIDataSource(
    web::WebUIIOS* web_ui,
    const std::string& name,
    base::span<const webui::ResourcePath> resource_paths,
    int html_resource_id) {
  web::WebUIIOSDataSource* source = web::WebUIIOSDataSource::Create(name);
  web::WebUIIOSDataSource::Add(GetProfile(web_ui), source);
  source->UseStringsJs();

  // Add required resources.
  source->AddResourcePaths(resource_paths);
  source->SetDefaultResource(html_resource_id);
  return source;
}

}  // namespace

NewTabTakeoverUI::NewTabTakeoverUI(web::WebUIIOS* web_ui, const GURL& url)
    : web::WebUIIOSController(web_ui, url.host()) {
   CreateAndAddWebUIDataSource(web_ui, url.host(),
                              base::span(kNewTabTakeoverResources),
                              IDR_NEW_TAB_TAKEOVER_NEW_TAB_TAKEOVER_PAGE_HTML);

  // Bind Mojom Interface
  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&NewTabTakeoverUI::BindInterface,
                          base::Unretained(this)));
}

NewTabTakeoverUI::~NewTabTakeoverUI() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      new_tab_takeover::mojom::NewTabTakeover::Name_);
}

void NewTabTakeoverUI::BindInterface(
    mojo::PendingReceiver<new_tab_takeover::mojom::NewTabTakeover> pending_receiver) {
  handler_.BindInterface(std::move(pending_receiver));
}
