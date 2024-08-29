/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_shields_api.h"

#include <optional>
#include <utility>

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ui/brave_pages.h"
#include "brave/common/extensions/api/brave_shields.h"
#include "brave/components/brave_shields/content/browser/ad_block_custom_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "content/public/browser/render_frame_host.h"

namespace extensions::api {

ExtensionFunction::ResponseAction
BraveShieldsAddSiteCosmeticFilterFunction::Run() {
  std::optional<brave_shields::AddSiteCosmeticFilter::Params> params =
      brave_shields::AddSiteCosmeticFilter::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  if (const auto* rfh = render_frame_host()) {
    g_brave_browser_process->ad_block_service()
        ->custom_filters_provider()
        ->HideElementOnHost(params->css_selector,
                            rfh->GetLastCommittedURL().host());
  }

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveShieldsOpenFilterManagementPageFunction::Run() {
  Browser* browser = chrome::FindLastActive();
  if (browser) {
    brave::ShowBraveAdblock(browser);
  }

  return RespondNow(NoArguments());
}

}  // namespace extensions::api
