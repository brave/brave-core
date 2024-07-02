// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_education/brave_education_ui.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/browser_command/brave_browser_command_handler.h"
#include "brave/components/brave_education/common/education_content_urls.h"
#include "brave/components/brave_education/resources/grit/brave_education_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/webui/web_ui_util.h"

namespace brave_education {

namespace {

class BraveEducationMessageHandler : public content::WebUIMessageHandler {
 public:
  BraveEducationMessageHandler() = default;
  ~BraveEducationMessageHandler() override = default;

  // content::WebUIMessageHandler:
  void RegisterMessages() override {
    web_ui()->RegisterMessageCallback(
        "initialize",
        base::BindRepeating(&BraveEducationMessageHandler::HandleInitialize,
                            base::Unretained(this)));
  }

 private:
  void HandleInitialize(const base::Value::List& args) {
    CHECK_EQ(2U, args.size());
    auto& callback_id = args[0].GetString();
    auto& url = args[1].GetString();

    AllowJavascript();

    std::string server_url;
    if (auto content_type = EducationContentTypeFromBrowserURL(url)) {
      server_url = GetEducationContentServerURL(*content_type).spec();
    }

    ResolveJavascriptCallback(base::Value(callback_id),
                              base::Value(std::move(server_url)));
  }
};

}  // namespace

// TODO(zenparsing): How do we get this to open up in a regular window always?

BraveEducationUI::BraveEducationUI(content::WebUI* web_ui,
                                   const std::string& host_name)
    : ui::MojoWebUIController(web_ui, /*enable_chrome_send=*/true),
      handler_factory_receiver_(this),
      profile_(Profile::FromWebUI(web_ui)) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile_, host_name);

  webui::SetupWebUIDataSource(
      source,
      base::make_span(kBraveEducationGenerated, kBraveEducationGeneratedSize),
      IDR_BRAVE_EDUCATION_HTML);

  auto* web_contents = web_ui->GetWebContents();

  AddBackgroundColorToSource(source, web_contents);

  // TODO(zenparsing): Add some strings for "title"?
  // static constexpr webui::LocalizedString kStrings[] = {};
  // source->AddLocalizedStrings(kStrings);

  // Allow embedding of iframe from brave.com
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ChildSrc,
      "child-src chrome://webui-test https://brave.com/;");

  web_ui->AddMessageHandler(std::make_unique<BraveEducationMessageHandler>());
}

BraveEducationUI::~BraveEducationUI() = default;

void BraveEducationUI::BindInterface(
    mojo::PendingReceiver<browser_command::mojom::CommandHandlerFactory>
        pending_receiver) {
  if (handler_factory_receiver_.is_bound()) {
    handler_factory_receiver_.reset();
  }
  handler_factory_receiver_.Bind(std::move(pending_receiver));
}

void BraveEducationUI::CreateBrowserCommandHandler(
    mojo::PendingReceiver<browser_command::mojom::CommandHandler>
        pending_handler) {
  std::vector supported_commands = {
      browser_command::mojom::Command::kOpenRewardsOnboarding,
      browser_command::mojom::Command::kOpenWalletOnboarding,
      browser_command::mojom::Command::kOpenWeb3Settings,
      browser_command::mojom::Command::kOpenContentFilterSettings,
      browser_command::mojom::Command::kOpenShieldsSettings,
      // TODO(zenparsing): Pending spec.
      // browser_command::mojom::Command::kOpenShieldsPanel,
      browser_command::mojom::Command::kOpenPrivacySettings,
      browser_command::mojom::Command::kOpenVPNOnboarding};

  command_handler_ = std::make_unique<BraveBrowserCommandHandler>(
      std::move(pending_handler), profile_, std::move(supported_commands));
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveEducationUI)

}  // namespace brave_education
