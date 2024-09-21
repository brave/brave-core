/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_wallet_api.h"

#include <optional>
#include <string>

#include "base/feature_list.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service_factory.h"
#include "brave/browser/ethereum_remote_client/features.h"
#include "brave/browser/ethereum_remote_client/pref_names.h"
#include "brave/browser/extensions/ethereum_remote_client_util.h"
#include "brave/common/extensions/api/brave_wallet.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_util.h"

namespace {

EthereumRemoteClientService* GetEthereumRemoteClientService(
    content::BrowserContext* context) {
  return EthereumRemoteClientServiceFactory::GetInstance()->GetForContext(
      context);
}

base::Value::Dict MakeSelectValue(const std::u16string& name,
                                  ::brave_wallet::mojom::DefaultWallet value) {
  base::Value::Dict item;
  item.Set("value", base::Value(static_cast<int>(value)));
  item.Set("name", base::Value(name));
  return item;
}

}  // namespace

namespace extensions::api {

ExtensionFunction::ResponseAction BraveWalletReadyFunction::Run() {
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

  ::brave_wallet::UpdateLastUnlockPref(g_browser_process->local_state());
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveWalletLoadUIFunction::Run() {
  auto* service = GetEthereumRemoteClientService(browser_context());
  // If the extension is already ready, respond right away
  if (service->IsCryptoWalletsReady()) {
    return RespondNow(NoArguments());
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* prefs = profile->GetPrefs();
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
  bool should_prompt =
      !profile->GetPrefs()->GetBoolean(kERCOptedIntoCryptoWallets);
  return RespondNow(WithArguments(should_prompt));
}

ExtensionFunction::ResponseAction BraveWalletGetWalletSeedFunction::Run() {
  // make sure the passed in enryption key is 32 bytes.
  std::optional<brave_wallet::GetWalletSeed::Params> params =
      brave_wallet::GetWalletSeed::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
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

  return RespondNow(WithArguments(base::Value(blob)));
}

ExtensionFunction::ResponseAction BraveWalletGetProjectIDFunction::Run() {
  std::string project_id = extensions::GetInfuraProjectID();
  return RespondNow(WithArguments(project_id));
}

ExtensionFunction::ResponseAction BraveWalletGetBraveKeyFunction::Run() {
  std::string brave_key = extensions::GetBraveKey();
  return RespondNow(WithArguments(brave_key));
}

ExtensionFunction::ResponseAction BraveWalletResetWalletFunction::Run() {
  auto* service = GetEthereumRemoteClientService(browser_context());
  service->ResetCryptoWallets();
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveWalletGetWeb3ProviderFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto default_wallet =
      ::brave_wallet::GetDefaultEthereumWallet(profile->GetPrefs());
  std::string extension_id;
  // This API is used so an extension can know when to prompt to
  // be the default Dapp provider. Since the new wallet is not an
  // extension at all, we can just re-use the Crypto Wallets ID.
  // We also don't want to prompt in Crypto Wallets when it's set
  // to Brave Wallet.
  if (default_wallet == ::brave_wallet::mojom::DefaultWallet::BraveWallet ||
      default_wallet ==
          ::brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension ||
      default_wallet == ::brave_wallet::mojom::DefaultWallet::CryptoWallets) {
    extension_id = kEthereumRemoteClientExtensionId;
  }
  return RespondNow(WithArguments(extension_id));
}

ExtensionFunction::ResponseAction
BraveWalletGetWeb3ProviderListFunction::Run() {
  base::Value::List list;
  list.Append(MakeSelectValue(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_WALLET_WEB3_PROVIDER_BRAVE_PREFER_EXTENSIONS),
      ::brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension));

  list.Append(
      MakeSelectValue(brave_l10n::GetLocalizedResourceUTF16String(
                          IDS_BRAVE_WALLET_WEB3_PROVIDER_BRAVE),
                      ::brave_wallet::mojom::DefaultWallet::BraveWallet));

  if (base::FeatureList::IsEnabled(ethereum_remote_client::features::
                                       kCryptoWalletsForNewInstallsFeature) ||
      extensions::ExtensionPrefs::Get(browser_context())
          ->HasPrefForExtension(kEthereumRemoteClientExtensionId)) {
    list.Append(MakeSelectValue(
        brave_l10n::GetLocalizedResourceUTF16String(
            IDS_BRAVE_WALLET_WEB3_PROVIDER_CRYPTO_WALLETS_DEPRECATED),
        ::brave_wallet::mojom::DefaultWallet::CryptoWallets));
  }

  list.Append(MakeSelectValue(brave_l10n::GetLocalizedResourceUTF16String(
                                  IDS_BRAVE_WALLET_WEB3_PROVIDER_NONE),
                              ::brave_wallet::mojom::DefaultWallet::None));
  std::string json_string;
  base::JSONWriter::Write(list, &json_string);
  return RespondNow(WithArguments(json_string));
}

ExtensionFunction::ResponseAction
BraveWalletIsNativeWalletEnabledFunction::Run() {
  return RespondNow(WithArguments(::brave_wallet::IsNativeWalletEnabled()));
}

}  // namespace extensions::api
