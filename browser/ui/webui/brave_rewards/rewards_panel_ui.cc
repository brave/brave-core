/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/rewards_panel_ui.h"

// TODO(zenparsing): We need to audit all of these includes
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_panel_new_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/base/webui/web_ui_util.h"

using brave_rewards::RewardsNotificationService;
using brave_rewards::RewardsNotificationServiceObserver;
using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;
using brave_rewards::RewardsServiceObserver;

namespace {

// TODO(zenparsing): Strings for this web UI will go here.
// static constexpr webui::LocalizedString kStrings[] = {};

class MessageHandler : public content::WebUIMessageHandler,
                       public RewardsNotificationServiceObserver,
                       public RewardsServiceObserver {
 public:
  MessageHandler() = default;
  ~MessageHandler() override = default;

  MessageHandler(const MessageHandler&) = delete;
  MessageHandler& operator=(const MessageHandler&) = delete;

  // WebUIMessageHandler:

  void OnJavascriptAllowed() override {
    if (auto* service = GetRewardsService())
      service->AddObserver(this);

    if (auto* service = GetRewardsNotificationService())
      service->AddObserver(this);
  }

  void OnJavascriptDisallowed() override {
    if (auto* service = GetRewardsService())
      service->RemoveObserver(this);

    if (auto* service = GetRewardsNotificationService())
      service->RemoveObserver(this);
  }

  void RegisterMessages() override {
    // Indicates that the page is ready to start receiving notifications from
    // the browser.
    web_ui()->RegisterMessageCallback(
        "PageReady",
        base::BindRepeating(&MessageHandler::OnPageReadyMessage,
                            base::Unretained(this)));

    web_ui()->RegisterMessageCallback(
        "ClosePanel",
        base::BindRepeating(&MessageHandler::OnClosePanelMessage,
                            base::Unretained(this)));
  }

 private:
  void OnPageReadyMessage(const base::ListValue* args) {
    AllowJavascript();

    if (auto* panel_ui = GetRewardsPanelUI())
      if (auto embedder = panel_ui->embedder())
        embedder->ShowUI();

    // Do initialization stuff. Start the rewards process? Whatever.
  }

  void OnClosePanelMessage(const base::ListValue* args) {
    AllowJavascript();

    if (auto* panel_ui = GetRewardsPanelUI())
      if (auto embedder = panel_ui->embedder())
        embedder->CloseUI();

    // Do initialization stuff. Start the rewards process? Whatever.
  }

  RewardsService* GetRewardsService() {
    return RewardsServiceFactory::GetForProfile(Profile::FromWebUI(web_ui()));
  }

  RewardsNotificationService* GetRewardsNotificationService() {
    if (auto* service = GetRewardsService())
      return service->GetNotificationService();

    return nullptr;
  }

  RewardsPanelUI* GetRewardsPanelUI() {
    if (auto* controller = web_ui()->GetController())
      return controller->template GetAs<RewardsPanelUI>();

    return nullptr;
  }
};

}  // namespace


RewardsPanelUI::RewardsPanelUI(content::WebUI* web_ui)
    : MojoBubbleWebUIController(web_ui, true) {
  auto* source = content::WebUIDataSource::Create(kBraveRewardsPanelHost);

  // TODO(zenparsing): This won't compile until we have at least one string
  // source->AddLocalizedStrings(kStrings);

  webui::SetupWebUIDataSource(source,
                              base::make_span(kBraveRewardsPanelNewGenerated,
                                              kBraveRewardsPanelNewGeneratedSize),
                              IDR_BRAVE_REWARDS_PANEL_NEW_HTML);

  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                source);

  auto* profile = Profile::FromWebUI(web_ui);

  // TODO(zenparsing): What's this for?
  content::URLDataSource::Add(profile, std::make_unique<FaviconSource>(
      profile, chrome::FaviconUrlFormat::kFavicon2));

  web_ui->AddMessageHandler(std::make_unique<MessageHandler>());
}

RewardsPanelUI::~RewardsPanelUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(RewardsPanelUI)
