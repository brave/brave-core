// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/extensions/api/brave_rewards_api.h"

#include "brave/browser/ui/brave_actions/brave_action_view_controller.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/common/extensions/api/brave_rewards.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "extensions/browser/event_router.h"

namespace extensions {
namespace api {


BraveRewardsOfferPaywallBypassFunction::~BraveRewardsOfferPaywallBypassFunction(
    ) {
}

ExtensionFunction::ResponseAction BraveRewardsOfferPaywallBypassFunction::Run(
    ) {
  std::unique_ptr<brave_rewards::OfferPaywallBypass::Params> params(
      brave_rewards::OfferPaywallBypass::Params::Create(*args_));

  EXTENSION_FUNCTION_VALIDATE(params.get());

  // get window tab is in
  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
          params->tab_id,
          Profile::FromBrowserContext(browser_context()),
          false,
          nullptr,
          nullptr,
          &contents,
          nullptr)) {
      LOG(ERROR) << "OfferPaywallBypass: Couldn't find WebContents for Tab";
      return RespondNow(Error(tabs_constants::kTabNotFoundError,
                            base::NumberToString(params->tab_id)));
  }
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  BraveLocationBarView* location_bar_view =
      static_cast<BraveLocationBarView*>(
          browser_view->toolbar()->location_bar());
  BraveRewardsActionViewController* extension_controller =
      static_cast<BraveRewardsActionViewController*>(
            location_bar_view->GetBraveActionsContainer()
            ->GetExtensionViewController(brave_rewards_extension_id));
  extension_controller->ExecuteOfferPaywallAction(params->publisher_id);
  LOG(ERROR) << "Offer paywall bypass!";
  return RespondNow(NoArguments());
}

BraveRewardsSendTipForPaywallBypassFunction::
    ~BraveRewardsSendTipForPaywallBypassFunction(
    ) { }

ExtensionFunction::ResponseAction
    BraveRewardsSendTipForPaywallBypassFunction::Run(
    ) {
  std::unique_ptr<brave_rewards::SendTipForPaywallBypass::Params> params(
      brave_rewards::SendTipForPaywallBypass::Params::Create(*args_));

  EXTENSION_FUNCTION_VALIDATE(params.get());


  LOG(ERROR) << "WALLBREAKER: Demo Tip sent for paywall bypass";

  auto* event_router = extensions::EventRouter::Get(
      Profile::FromBrowserContext(browser_context()));
  if (!event_router) {
    LOG(ERROR) << "WALLBREAKER: No event router!";
    return RespondNow(NoArguments());
  }

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnPaywallBypassRequested::Create(
          params->publisher_id).release());
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnPaywallBypassRequested::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));

  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
