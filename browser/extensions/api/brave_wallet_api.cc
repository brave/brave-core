/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_wallet_api.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service_factory.h"
#include "brave/browser/ethereum_remote_client/pref_names.h"
#include "brave/browser/extensions/ethereum_remote_client_util.h"
#include "brave/common/extensions/api/brave_wallet.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/constants.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

EthereumRemoteClientService* GetEthereumRemoteClientService(
    content::BrowserContext* context) {
  return EthereumRemoteClientServiceFactory::GetInstance()->GetForContext(
      context);
}

base::Value MakeSelectValue(const std::u16string& name,
                            ::brave_wallet::mojom::DefaultWallet value) {
  base::Value item(base::Value::Type::DICTIONARY);
  item.SetKey("value", base::Value(static_cast<int>(value)));
  item.SetKey("name", base::Value(name));
  return item;
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
BraveWalletReadyFunction::Run() {
  if (browser_context()->IsTor()) {
    return RespondNow(Error("Not available in Tor context"));
  }

  auto* service = GetEthereumRemoteClientService(browser_context());
  service->CryptoWalletsExtensionReady();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveWalletNotifyWalletUnlockFunction::Run() {
  if (browser_context()->IsTor()) {
    return RespondNow(Error("Not available in Tor context"));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  ::brave_wallet::UpdateLastUnlockPref(profile->GetPrefs());

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveWalletLoadUIFunction::Run() {
  auto* service = GetEthereumRemoteClientService(browser_context());
  // If the extension is already ready, respond right away
  if (service->IsCryptoWalletsReady()) {
    return RespondNow(NoArguments());
  }

  // If the user has opt-ed in and MetaMask is not installed, and
  // the new Brave Wallet is not the default, then
  // set the Dapp provider to Crypto Wallets.
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* prefs = profile->GetPrefs();
  auto default_wallet = ::brave_wallet::GetDefaultWallet(prefs);
  auto* registry = extensions::ExtensionRegistry::Get(profile);
  if (!registry->ready_extensions().Contains(metamask_extension_id) &&
      default_wallet != ::brave_wallet::mojom::DefaultWallet::BraveWallet) {
    ::brave_wallet::SetDefaultWallet(
        prefs, ::brave_wallet::mojom::DefaultWallet::CryptoWallets);
  }
  prefs->SetBoolean(kERCOptedIntoCryptoWallets, true);
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
  auto* service = GetEthereumRemoteClientService(browser_context());
  bool should_prompt =
      !service->IsLegacyCryptoWalletsSetup() &&
      !profile->GetPrefs()->GetBoolean(kERCOptedIntoCryptoWallets);
  return RespondNow(OneArgument(base::Value(should_prompt)));
}

ExtensionFunction::ResponseAction
BraveWalletGetWalletSeedFunction::Run() {
  // make sure the passed in enryption key is 32 bytes.
  std::unique_ptr<brave_wallet::GetWalletSeed::Params> params(
    brave_wallet::GetWalletSeed::Params::Create(*args_));
  if (params->key.size() != 32) {
    return RespondNow(Error("Invalid input key size"));
  }

  auto* service = GetEthereumRemoteClientService(browser_context());

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

  auto* service = GetEthereumRemoteClientService(browser_context());

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
  auto* service = GetEthereumRemoteClientService(browser_context());
  service->ResetCryptoWallets();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveWalletGetWeb3ProviderFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto default_wallet = ::brave_wallet::GetDefaultWallet(profile->GetPrefs());
  std::string extension_id;
  if (default_wallet == ::brave_wallet::mojom::DefaultWallet::BraveWallet) {
    // This API is used so an extension can know when to prompt to
    // be the default Dapp provider. Since the new wallet is not an
    // extension at all, we can just re-use the Crypto Wallets ID.
    // We also don't want to prompt in Crypto Wallets when it's set
    // to Brave Wallet.
    extension_id = ethereum_remote_client_extension_id;
  } else if (default_wallet ==
             ::brave_wallet::mojom::DefaultWallet::CryptoWallets) {
    extension_id = ethereum_remote_client_extension_id;
  } else if (default_wallet == ::brave_wallet::mojom::DefaultWallet::Metamask) {
    extension_id = metamask_extension_id;
  }
  return RespondNow(OneArgument(base::Value(extension_id)));
}

ExtensionFunction::ResponseAction
BraveWalletGetWeb3ProviderListFunction::Run() {
  base::Value list(base::Value::Type::LIST);
  // There is no Ask mode in the new wallet flow, instead it is
  // just defaulted to the new wallet since there is no overhead.
  bool new_wallet = ::brave_wallet::IsNativeWalletEnabled();
  if (new_wallet) {
    list.Append(MakeSelectValue(
        l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_BRAVE),
        ::brave_wallet::mojom::DefaultWallet::BraveWallet));
  } else {
    list.Append(MakeSelectValue(
        l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_ASK),
        ::brave_wallet::mojom::DefaultWallet::Ask));
  }
  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(
          new_wallet ? IDS_BRAVE_WALLET_WEB3_PROVIDER_CRYPTO_WALLETS_DEPRECATED
                     : IDS_BRAVE_WALLET_WEB3_PROVIDER_CRYPTO_WALLETS),
      ::brave_wallet::mojom::DefaultWallet::CryptoWallets));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* registry = extensions::ExtensionRegistry::Get(profile);
  if (registry->ready_extensions().Contains(metamask_extension_id)) {
    list.Append(MakeSelectValue(
        l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_METAMASK),
        ::brave_wallet::mojom::DefaultWallet::Metamask));
  }
  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_NONE),
      ::brave_wallet::mojom::DefaultWallet::None));
  std::string json_string;
  base::JSONWriter::Write(list, &json_string);
  return RespondNow(OneArgument(base::Value(json_string)));
}

ExtensionFunction::ResponseAction
BraveWalletIsNativeWalletEnabledFunction::Run() {
  return RespondNow(
      OneArgument(base::Value(::brave_wallet::IsNativeWalletEnabled())));
}

}  // namespace api
}  // namespace extensions
