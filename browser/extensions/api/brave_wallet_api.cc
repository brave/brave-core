/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_wallet_api.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/extensions/brave_wallet_util.h"
#include "brave/browser/infobars/crypto_wallets_infobar_delegate.h"
#include "brave/common/extensions/api/brave_wallet.h"
#include "brave/components/brave_wallet/brave_wallet_constants.h"
#include "brave/components/brave_wallet/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/constants.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/brave_wallet_service.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

BraveWalletService* GetBraveWalletService(
    content::BrowserContext* context) {
  return BraveWalletServiceFactory::GetInstance()
      ->GetForProfile(Profile::FromBrowserContext(context));
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

  if (browser_context()->IsTor()) {
    return RespondNow(Error("Not available in Tor context"));
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
    CryptoWalletsInfoBarDelegate::InfobarSubType subtype =
        CryptoWalletsInfoBarDelegate::InfobarSubType::GENERIC_SETUP;
    auto* service = GetBraveWalletService(browser_context());
    if (service->ShouldShowLazyLoadInfobar()) {
      subtype = CryptoWalletsInfoBarDelegate::InfobarSubType::
          LOAD_CRYPTO_WALLETS;
    }
    CryptoWalletsInfoBarDelegate::Create(infobar_service, subtype);
  }

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveWalletReadyFunction::Run() {
  if (browser_context()->IsTor()) {
    return RespondNow(Error("Not available in Tor context"));
  }

  auto* service = GetBraveWalletService(browser_context());
  service->CryptoWalletsExtensionReady();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveWalletLoadUIFunction::Run() {
  auto* service = GetBraveWalletService(browser_context());
  // If the extension is already ready, respond right away
  if (service->IsCryptoWalletsReady()) {
    return RespondNow(NoArguments());
  }

  // If the user has opt-ed in and MetaMask is not installed, then
  // set the Dapp provider to Crypto Wallets.
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* registry = extensions::ExtensionRegistry::Get(profile);
  if (!registry->ready_extensions().Contains(metamask_extension_id)) {
    profile->GetPrefs()->SetInteger(kBraveWalletWeb3Provider,
        static_cast<int>(BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS));
  }

  profile->GetPrefs()->SetBoolean(kOptedIntoCryptoWallets, true);
  service->MaybeLoadCryptoWalletsExtension(
      base::BindOnce(&BraveWalletLoadUIFunction::OnLoaded, this));
  return RespondLater();
}

void BraveWalletLoadUIFunction::OnLoaded() {
  Respond(NoArguments());
}

ExtensionFunction::ResponseAction
BraveWalletShouldPromptForSetupFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* service = BraveWalletServiceFactory::GetForProfile(profile);
  bool should_prompt = !service->IsCryptoWalletsSetup() &&
      !profile->GetPrefs()->GetBoolean(kOptedIntoCryptoWallets);
  return RespondNow(OneArgument(base::Value(should_prompt)));
}

ExtensionFunction::ResponseAction
BraveWalletShouldCheckForDappsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (browser_context()->IsTor()) {
    return RespondNow(OneArgument(base::Value(false)));
  }
  auto provider = static_cast<BraveWalletWeb3ProviderTypes>(
      profile->GetPrefs()->GetInteger(kBraveWalletWeb3Provider));
  auto* registry = extensions::ExtensionRegistry::Get(profile);
  bool has_metamask =
      registry->ready_extensions().Contains(metamask_extension_id);

  auto* service = GetBraveWalletService(browser_context());
  bool dappDetection = (
      provider == BraveWalletWeb3ProviderTypes::ASK && !has_metamask) ||
      (provider == BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS &&
       !service->IsCryptoWalletsReady());

  return RespondNow(OneArgument(base::Value(dappDetection)));
}

ExtensionFunction::ResponseAction
BraveWalletGetWalletSeedFunction::Run() {
  // make sure the passed in enryption key is 32 bytes.
  std::unique_ptr<brave_wallet::GetWalletSeed::Params> params(
    brave_wallet::GetWalletSeed::Params::Create(*args_));
  if (params->key.size() != 32) {
    return RespondNow(Error("Invalid input key size"));
  }

  auto* service = GetBraveWalletService(browser_context());

  base::Value::BlobStorage blob;
  std::string derived = service->GetWalletSeed(params->key);

  if (derived.empty()) {
    return RespondNow(Error("Error getting wallet seed"));
  }

  blob.assign(derived.begin(), derived.end());

  return RespondNow(OneArgument(base::Value(blob)));
}

ExtensionFunction::ResponseAction
BraveWalletGetBitGoSeedFunction::Run() {
  // make sure the passed in enryption key is 32 bytes.
  std::unique_ptr<brave_wallet::GetBitGoSeed::Params> params(
    brave_wallet::GetBitGoSeed::Params::Create(*args_));
  if (params->key.size() != 32) {
    return RespondNow(Error("Invalid input key size"));
  }

  auto* service = GetBraveWalletService(browser_context());

  base::Value::BlobStorage blob;
  std::string derived = service->GetBitGoSeed(params->key);

  if (derived.empty()) {
    return RespondNow(Error("Error getting wallet seed"));
  }

  blob.assign(derived.begin(), derived.end());

  return RespondNow(OneArgument(base::Value(blob)));
}

ExtensionFunction::ResponseAction
BraveWalletGetProjectIDFunction::Run() {
  std::string project_id = extensions::GetInfuraProjectID();
  return RespondNow(OneArgument(base::Value(project_id)));
}

ExtensionFunction::ResponseAction
BraveWalletGetBraveKeyFunction::Run() {
  std::string brave_key = extensions::GetBraveKey();
  return RespondNow(OneArgument(base::Value(brave_key)));
}

ExtensionFunction::ResponseAction
BraveWalletResetWalletFunction::Run() {
  auto* service = GetBraveWalletService(browser_context());
  service->ResetCryptoWallets();
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
  return RespondNow(OneArgument(base::Value(extension_id)));
}

ExtensionFunction::ResponseAction
BraveWalletGetWeb3ProviderListFunction::Run() {
  base::Value list(base::Value::Type::LIST);
  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_ASK),
      BraveWalletWeb3ProviderTypes::ASK));
  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_NONE),
      BraveWalletWeb3ProviderTypes::NONE));
  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_CRYPTO_WALLETS),
      BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* registry = extensions::ExtensionRegistry::Get(profile);
  if (registry->ready_extensions().Contains(metamask_extension_id)) {
    list.Append(MakeSelectValue(
        l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_METAMASK),
        BraveWalletWeb3ProviderTypes::METAMASK));
  }
  std::string json_string;
  base::JSONWriter::Write(list, &json_string);
  return RespondNow(OneArgument(base::Value(json_string)));
}

}  // namespace api
}  // namespace extensions
