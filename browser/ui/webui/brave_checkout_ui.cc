/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_checkout_ui.h"

#include "brave/browser/ui/webui/basic_ui.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_checkout_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui_data_source.h"

BraveCheckoutUI::BraveCheckoutUI(
    content::WebUI* web_ui,
    const std::string& name)
    : ConstrainedWebDialogUI(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, CreateBasicUIHTMLSource(
      profile,
      name,
      kBraveRewardsCheckoutGenerated,
      kBraveRewardsCheckoutGeneratedSize,
      IDR_BRAVE_REWARDS_CHECKOUT_HTML));
}

BraveCheckoutUI::~BraveCheckoutUI() = default;
