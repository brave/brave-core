/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/rewards_page_top_ui.h"

#include <utility>

#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_page_data_source.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_page_handler.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_web_ui_utils.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "content/public/common/url_constants.h"

namespace brave_rewards {

namespace {

class RewardsPageBubbleDelegate : public RewardsPageHandler::BubbleDelegate {
 public:
  RewardsPageBubbleDelegate(
      base::WeakPtr<Profile> profile,
      base::WeakPtr<TopChromeWebUIController::Embedder> embedder)
      : profile_(profile), embedder_(embedder) {}

  void ShowUI() override {
    if (embedder_) {
      embedder_->ShowUI();
    }
  }

  void OpenTab(const std::string& url) override {
    GURL target_url(url);
    if (!target_url.is_valid() || !profile_) {
      return;
    }
    if (auto* browser = chrome::FindLastActiveWithProfile(profile_.get())) {
      chrome::AddTabAt(browser, target_url, -1, true);
    }
  }

 private:
  base::WeakPtr<Profile> profile_;
  base::WeakPtr<TopChromeWebUIController::Embedder> embedder_;
};

}  // namespace

RewardsPageTopUI::RewardsPageTopUI(content::WebUI* web_ui)
    : TopChromeWebUIController(web_ui) {
  CreateAndAddRewardsPageDataSource(*web_ui, kRewardsPageTopHost);
}

RewardsPageTopUI::~RewardsPageTopUI() = default;

void RewardsPageTopUI::BindInterface(
    mojo::PendingReceiver<RewardsPageHandlerFactory> receiver) {
  factory_receiver_.reset();
  factory_receiver_.Bind(std::move(receiver));
}

void RewardsPageTopUI::CreatePageHandler(
    mojo::PendingRemote<mojom::RewardsPage> page,
    mojo::PendingReceiver<mojom::RewardsPageHandler> handler) {
  DCHECK(page);

  auto* profile = Profile::FromWebUI(web_ui());
  CHECK(profile);
  auto bubble_delegate = std::make_unique<RewardsPageBubbleDelegate>(
      profile->GetWeakPtr(), embedder());

  handler_ = std::make_unique<RewardsPageHandler>(
      std::move(page), std::move(handler), std::move(bubble_delegate),
      Profile::FromWebUI(web_ui()));
}

WEB_UI_CONTROLLER_TYPE_IMPL(RewardsPageTopUI)

RewardsPageTopUIConfig::RewardsPageTopUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIScheme,
                                  kRewardsPageTopHost) {}

bool RewardsPageTopUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return !ShouldBlockRewardsWebUI(browser_context, GURL(kRewardsPageTopURL));
}

bool RewardsPageTopUIConfig::ShouldAutoResizeHost() {
  return true;
}

}  // namespace brave_rewards
