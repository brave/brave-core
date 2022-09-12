/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_panel/rewards_panel_extension_handler.h"

#include <memory>
#include <string>

#include "base/strings/strcat.h"
#include "brave/browser/extensions/api/brave_action_api.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_system.h"

namespace brave_rewards {

namespace {

constexpr char kRewardsPanelUrl[] = "/brave_rewards_panel.html";

std::string GetExtensionPath(const mojom::RewardsPanelArgs& args) {
  switch (args.view) {
    case mojom::RewardsPanelView::kDefault:
      return kRewardsPanelUrl;
    case mojom::RewardsPanelView::kRewardsTour:
      return base::StrCat({kRewardsPanelUrl, "#tour"});
    case mojom::RewardsPanelView::kGrantCaptcha:
      return base::StrCat({kRewardsPanelUrl, "#grant_", args.data});
    case mojom::RewardsPanelView::kAdaptiveCaptcha:
      return base::StrCat({kRewardsPanelUrl, "#load_adaptive_captcha"});
  }
}

}  // namespace

RewardsPanelExtensionHandler::RewardsPanelExtensionHandler(
    Browser* browser,
    RewardsService* rewards_service)
    : browser_(browser), rewards_service_(rewards_service) {
  DCHECK(browser_);
  DCHECK(rewards_service_);
}

RewardsPanelExtensionHandler::~RewardsPanelExtensionHandler() = default;

bool RewardsPanelExtensionHandler::IsRewardsExtensionPanelURL(const GURL& url) {
  return url.SchemeIs("chrome-extension") &&
         url.host() == brave_rewards_extension_id &&
         url.path() == kRewardsPanelUrl;
}

void RewardsPanelExtensionHandler::OnRewardsPanelRequested(
    const mojom::RewardsPanelArgs& args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->StartProcess(base::DoNothing());

  auto* profile = browser_->profile();

  // Load the rewards extension if it is not already loaded.
  auto* extension_service =
      extensions::ExtensionSystem::Get(profile)->extension_service();
  if (!extension_service) {
    return;
  }

  static_cast<extensions::BraveComponentLoader*>(
      extension_service->component_loader())
      ->AddRewardsExtension();

  auto result = extensions::BraveActionAPI::ShowActionUI(
      browser_, brave_rewards_extension_id, GetExtensionPath(args));
  if (!result.has_value()) {
    LOG(ERROR) << "Failure to show Action UI. error=" << result.error();
  }
}

}  // namespace brave_rewards
