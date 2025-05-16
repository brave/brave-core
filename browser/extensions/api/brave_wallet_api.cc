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
#include "brave/common/extensions/api/brave_wallet.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_util.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

base::Value::Dict MakeSelectValue(const std::u16string& name,
                                  ::brave_wallet::mojom::DefaultWallet value) {
  base::Value::Dict item;
  item.Set("value", base::Value(static_cast<int>(value)));
  item.Set("name", base::Value(name));
  return item;
}

}  // namespace

namespace extensions::api {

ExtensionFunction::ResponseAction
BraveWalletGetWeb3ProviderListFunction::Run() {
  base::Value::List list;
  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(
          IDS_BRAVE_WALLET_WEB3_PROVIDER_BRAVE_PREFER_EXTENSIONS),
      ::brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension));

  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_BRAVE),
      ::brave_wallet::mojom::DefaultWallet::BraveWallet));

  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_BRAVE_WALLET_WEB3_PROVIDER_NONE),
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
