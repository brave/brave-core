/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_wallet_api.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/infobars/crypto_wallets_infobar_delegate.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/brave_wallet_constants.h"
#include "brave/common/extensions/api/brave_wallet.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_controller.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/browser/extensions/brave_wallet_util.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

BraveWalletController* GetBraveWalletController(
    content::BrowserContext* context) {
  return BraveWalletServiceFactory::GetInstance()
      ->GetForProfile(Profile::FromBrowserContext(context))
      ->controller();
}

base::Value MakeSelectValue(const  base::string16& name,
                            BraveWalletWeb3ProviderTypes value) {
  base::Value item(base::Value::Type::DICTIONARY);
  item.SetKey("value", base::Value(static_cast<int>(value)));
  item.SetKey("name", base::Value(name));
  return item;
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
BraveWalletPromptToEnableWalletFunction::Run() {
  std::unique_ptr<brave_wallet::PromptToEnableWallet::Params> params(
      brave_wallet::PromptToEnableWallet::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

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

  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(contents);
  if (infobar_service) {
    auto* registry = extensions::ExtensionRegistry::Get(profile);
    bool metamask_enabled = !brave::IsTorProfile(profile) &&
      registry->ready_extensions().GetByID(metamask_extension_id);
    CryptoWalletsInfoBarDelegate::Create(infobar_service, metamask_enabled);
  }

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveWalletIsInstalledFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* registry = extensions::ExtensionRegistry::Get(profile);
  bool enabled = !brave::IsTorProfile(profile) &&
    registry->ready_extensions().GetByID(ethereum_remote_client_extension_id);
  return RespondNow(OneArgument(std::make_unique<base::Value>(enabled)));
}

ExtensionFunction::ResponseAction
BraveWalletShouldCheckForDappsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto provider = static_cast<BraveWalletWeb3ProviderTypes>(
      profile->GetPrefs()->GetInteger(kBraveWalletWeb3Provider));
  bool dappDetection = !brave::IsTorProfile(profile);
  if (provider != BraveWalletWeb3ProviderTypes::ASK) {
    dappDetection = false;
  }
  return RespondNow(OneArgument(
      std::make_unique<base::Value>(dappDetection)));
}

ExtensionFunction::ResponseAction
BraveWalletGetWalletSeedFunction::Run() {
  // make sure the passed in enryption key is 32 bytes.
  std::unique_ptr<brave_wallet::GetWalletSeed::Params> params(
    brave_wallet::GetWalletSeed::Params::Create(*args_));
  if (params->key.size() != 32) {
    return RespondNow(Error("Invalid input key size"));
  }

  auto* controller = GetBraveWalletController(browser_context());

  base::Value::BlobStorage blob;
  std::string derived = controller->GetWalletSeed(params->key);

  if (derived.empty()) {
    return RespondNow(Error("Error getting wallet seed"));
  }

  blob.assign(derived.begin(), derived.end());

  return RespondNow(OneArgument(
    std::make_unique<base::Value>(blob)));
}

ExtensionFunction::ResponseAction
BraveWalletGetBitGoSeedFunction::Run() {
  // make sure the passed in enryption key is 32 bytes.
  std::unique_ptr<brave_wallet::GetBitGoSeed::Params> params(
    brave_wallet::GetBitGoSeed::Params::Create(*args_));
  if (params->key.size() != 32) {
    return RespondNow(Error("Invalid input key size"));
  }

  auto* controller = GetBraveWalletController(browser_context());

  base::Value::BlobStorage blob;
  std::string derived = controller->GetBitGoSeed(params->key);

  if (derived.empty()) {
    return RespondNow(Error("Error getting wallet seed"));
  }

  blob.assign(derived.begin(), derived.end());

  return RespondNow(OneArgument(
    std::make_unique<base::Value>(blob)));
}

ExtensionFunction::ResponseAction
BraveWalletGetProjectIDFunction::Run() {
  std::string project_id = extensions::GetInfuraProjectID();
  return RespondNow(OneArgument(
      std::make_unique<base::Value>(project_id)));
}

ExtensionFunction::ResponseAction
BraveWalletResetWalletFunction::Run() {
  auto* controller = GetBraveWalletController(browser_context());
  controller->ResetCryptoWallets();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveWalletGetWeb3ProviderFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto provider = static_cast<BraveWalletWeb3ProviderTypes>(
      profile->GetPrefs()->GetInteger(kBraveWalletWeb3Provider));
  std::string extension_id;
  if (provider == BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS) {
    extension_id = ethereum_remote_client_extension_id;
  } else if (provider == BraveWalletWeb3ProviderTypes::METAMASK) {
    extension_id = metamask_extension_id;
  }
  return RespondNow(OneArgument(
      std::make_unique<base::Value>(extension_id)));
}

ExtensionFunction::ResponseAction
BraveWalletGetWeb3ProviderListFunction::Run() {
  base::Value list(base::Value::Type::LIST);
  list.GetList().push_back(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_ASK),
      BraveWalletWeb3ProviderTypes::ASK));
  list.GetList().push_back(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_NONE),
      BraveWalletWeb3ProviderTypes::NONE));
  list.GetList().push_back(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_CRYPTO_WALLETS),
      BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* registry = extensions::ExtensionRegistry::Get(profile);
  if (registry->ready_extensions().GetByID(metamask_extension_id)) {
    list.GetList().push_back(MakeSelectValue(
        l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_METAMASK),
        BraveWalletWeb3ProviderTypes::METAMASK));
  }
  std::string json_string;
  base::JSONWriter::Write(list, &json_string);
  return RespondNow(OneArgument(std::make_unique<base::Value>(json_string)));
}

}  // namespace api
}  // namespace extensions
