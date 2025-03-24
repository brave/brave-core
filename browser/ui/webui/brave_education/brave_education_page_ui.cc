// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// based on //chrome/browser/ui/webui/whats_new/whats_new_ui.cc

#include "brave/browser/ui/webui/brave_education/brave_education_page_ui.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/browser/ui/webui/brave_browser_command/brave_browser_command_handler.h"
#include "brave/browser/ui/webui/brave_education/brave_education_handler.h"
#include "brave/browser/ui/webui/brave_education/brave_education_page_delegate_desktop.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_education/education_urls.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/grit/brave_education_resources.h"
#include "brave/grit/brave_education_resources_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/branded_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/tab_collections/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/webui_util.h"

namespace {

void CreateAndAddWhatsNewUIHtmlSource(content::WebUI* web_ui,
                                      Profile* profile) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kBraveGettingStartedHost);
  webui::SetupWebUIDataSource(
      source, base::span<const webui::ResourcePath>(kBraveEducationResources),
      IDR_BRAVE_EDUCATION_BRAVE_EDUCATION_HTML);

  AddBackgroundColorToSource(source, web_ui->GetWebContents());

  static constexpr webui::LocalizedString kStrings[] = {
      {"title", IDS_WELCOME_HEADER}};
  source->AddLocalizedStrings(kStrings);

  // Allow embedding of iframe content from allowed domains.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ChildSrc,
      "child-src chrome://webui-test https://browser-education.brave.com;");
}

}  // namespace

BraveEducationPageUI::BraveEducationPageUI(content::WebUI* web_ui,
                                           const GURL& url)
    : ui::MojoWebUIController(web_ui, /*enable_chrome_send=*/true),
      page_factory_receiver_(this),
      browser_command_factory_receiver_(this),
      profile_(Profile::FromWebUI(web_ui)) {
  CreateAndAddWhatsNewUIHtmlSource(web_ui, profile_);
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveEducationPageUI)

void BraveEducationPageUI::BindInterface(
    mojo::PendingReceiver<brave_education::mojom::PageHandlerFactory>
        receiver) {
  page_factory_receiver_.reset();
  page_factory_receiver_.Bind(std::move(receiver));
}

void BraveEducationPageUI::CreatePageHandler(
    mojo::PendingRemote<brave_education::mojom::Page> page,
    mojo::PendingReceiver<brave_education::mojom::PageHandler> receiver) {
  DCHECK(page);

  auto* web_contents = web_ui()->GetWebContents();
  auto education_page_type = brave_education::EducationPageTypeFromBrowserURL(
      web_contents->GetVisibleURL());

  page_handler_ = std::make_unique<BraveEducationHandler>(
      std::move(receiver), std::move(page), *education_page_type);
}

void BraveEducationPageUI::BindInterface(
    mojo::PendingReceiver<BraveBrowserCommandHandlerFactory> pending_receiver) {
  if (browser_command_factory_receiver_.is_bound()) {
    browser_command_factory_receiver_.reset();
  }
  browser_command_factory_receiver_.Bind(std::move(pending_receiver));
}

void BraveEducationPageUI::CreateBrowserCommandHandler(
    mojo::PendingReceiver<
        brave_browser_command::mojom::BraveBrowserCommandHandler>
        pending_handler) {
  std::vector<brave_browser_command::mojom::Command> supported_commands = {};

  supported_commands.insert(
      supported_commands.end(),
      {brave_browser_command::mojom::Command::kOpenRewardsOnboarding,
       brave_browser_command::mojom::Command::kOpenWalletOnboarding,
       brave_browser_command::mojom::Command::kOpenVPNOnboarding,
       brave_browser_command::mojom::Command::kOpenAIChat});

  auto* web_contents = web_ui()->GetWebContents();
  auto* tab = tabs::TabInterface::GetFromContents(web_contents);
  CHECK(tab);

  command_handler_ = std::make_unique<BraveBrowserCommandHandler>(
      std::move(pending_handler), profile_, supported_commands,
      std::make_unique<brave_education::BraveEducationPageDelegateDesktop>(
          *tab->GetBrowserWindowInterface()));
}

BraveEducationPageUI::~BraveEducationPageUI() = default;
