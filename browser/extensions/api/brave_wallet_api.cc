/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_wallet_api.h"

#include <memory>

#include "brave/browser/dapp/dapp_utils.h"
#include "brave/common/extensions/api/brave_wallet.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
BraveWalletPromptToEnableWalletFunction::Run() {
  std::unique_ptr<brave_wallet::PromptToEnableWallet::Params> params(
      brave_wallet::PromptToEnableWallet::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // Get web contents for this tab
  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
        params->tab_id,
        Profile::FromBrowserContext(browser_context()),
        include_incognito_information(),
        nullptr,
        nullptr,
        &contents,
        nullptr)) {
    return RespondNow(Error(tabs_constants::kTabNotFoundError,
                            base::NumberToString(params->tab_id)));
  }

  RequestWalletInstallationPermission(contents);
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveWalletIsEnabledFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* registry = extensions::ExtensionRegistry::Get(profile);
  bool enabled =
    registry->ready_extensions().GetByID(ethereum_remote_client_extension_id);
  return RespondNow(OneArgument(std::make_unique<base::Value>(enabled)));
}

}  // namespace api
}  // namespace extensions
