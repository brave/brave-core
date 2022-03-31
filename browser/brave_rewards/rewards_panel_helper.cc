/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_panel_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "extensions/browser/extension_system.h"

namespace {
constexpr char kAdaptiveCaptchaPanelUrl[] = "adaptive_captcha_panel.html";
constexpr char kRewardsPanelUrl[] = "brave_rewards_panel.html";

bool ShowPanel(content::BrowserContext* context,
               const std::string& relative_path) {
  Profile* profile = Profile::FromBrowserContext(context);
  Browser* browser = chrome::FindTabbedBrowser(profile, false);
  if (!browser) {
    return false;
  }

  // Start the rewards ledger process if it is not already started
  auto* rewards_service =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return false;
  }

  rewards_service->StartProcess(base::DoNothing());

  // Load the rewards extension if it is not already loaded
  auto* extension_service =
      extensions::ExtensionSystem::Get(profile)->extension_service();
  if (!extension_service) {
    return false;
  }

  static_cast<extensions::BraveComponentLoader*>(
      extension_service->component_loader())
      ->AddRewardsExtension();

  if (browser->window()->IsMinimized()) {
    browser->window()->Restore();
  }

  std::string error;
  bool popup_shown = extensions::BraveActionAPI::ShowActionUI(
      browser, brave_rewards_extension_id,
      std::make_unique<std::string>(relative_path), &error);
  if (!popup_shown) {
    return false;
  }

  return true;
}

}  // namespace

namespace brave_rewards {

bool ShowRewardsPanel(content::BrowserContext* context) {
  return ShowPanel(context, kRewardsPanelUrl);
}

bool ShowAdaptiveCaptchaPanel(content::BrowserContext* context) {
  return ShowPanel(context, kAdaptiveCaptchaPanelUrl);
}

}  // namespace brave_rewards
