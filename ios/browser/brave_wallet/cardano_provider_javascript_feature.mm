// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_wallet/cardano_provider_javascript_feature.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/fixed_flat_map.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/notreached.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_utils.h"
#include "brave/ios/browser/brave_wallet/cardano_provider_tab_helper.h"
#include "brave/ios/web/js_messaging/message_handler_token.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/navigation/navigation_item.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/security/ssl_status.h"
#include "ios/web/public/web_state.h"
#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

constexpr char kCardanoProviderJavaScriptFeatureKeyName[] =
    "cardano_provider_java_script_feature";
constexpr char kScriptName[] = "cardano_provider";
constexpr char kScriptHandlerName[] = "CardanoProviderMessageHandler";

constexpr char kMethodKey[] = "method";
constexpr char kArgsKey[] = "args";

// See "Error Types" from https://cips.cardano.org/cip/CIP-30
constexpr int kAPIErrorInternalError = -2;

// The `method` value posted by cardano_provider.ts.
enum class ProviderMethod {
  kIsEnabled,
  kEnable,
  kGetNetworkId,
  kGetUsedAddresses,
  kGetUnusedAddresses,
  kGetChangeAddress,
  kGetRewardAddresses,
  kGetBalance,
  kGetUtxos,
  kSignTx,
  kSignData,
  kSubmitTx,
  kGetCollateral,
};

using ScriptMessageReplyCallback =
    base::OnceCallback<void(const base::Value* reply, NSString* error)>;

std::optional<ProviderMethod> ParseMethod(std::string_view method) {
  static constexpr auto kMethods =
      base::MakeFixedFlatMap<std::string_view, ProviderMethod>({
          {"isEnabled", ProviderMethod::kIsEnabled},
          {"enable", ProviderMethod::kEnable},
          {"getNetworkId", ProviderMethod::kGetNetworkId},
          {"getUsedAddresses", ProviderMethod::kGetUsedAddresses},
          {"getUnusedAddresses", ProviderMethod::kGetUnusedAddresses},
          {"getChangeAddress", ProviderMethod::kGetChangeAddress},
          {"getRewardAddresses", ProviderMethod::kGetRewardAddresses},
          {"getBalance", ProviderMethod::kGetBalance},
          {"getUtxos", ProviderMethod::kGetUtxos},
          {"signTx", ProviderMethod::kSignTx},
          {"signData", ProviderMethod::kSignData},
          {"submitTx", ProviderMethod::kSubmitTx},
          {"getCollateral", ProviderMethod::kGetCollateral},
      });
  auto it = kMethods.find(method);
  if (it == kMethods.end()) {
    return std::nullopt;
  }
  return it->second;
}

// Serializes a `CardanoProviderErrorBundle` into the JSON error object the page
// rejects with. cardano_provider.ts strips the `Error: ` prefix and parses it.
// Matches the desktop renderer's `ConvertError` shape (see
// `js_cardano_wallet_api.cc`).
NSString* SerializeError(const mojom::CardanoProviderErrorBundlePtr& error) {
  base::DictValue dict;
  if (error && error->pagination_error_payload) {
    dict.Set("maxSize", error->pagination_error_payload->payload);
  } else if (error) {
    dict.Set("code", error->code);
    dict.Set("info", error->error_message);
  } else {
    dict.Set("code", kAPIErrorInternalError);
    dict.Set("info", "Internal error");
  }
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return base::SysUTF8ToNSString(json);
}

// Resolves with a string result, or rejects with the error bundle.
void OnStringResult(ScriptMessageReplyCallback callback,
                    const std::optional<std::string>& result,
                    mojom::CardanoProviderErrorBundlePtr error) {
  if (result) {
    base::Value value(*result);
    std::move(callback).Run(&value, nil);
  } else {
    std::move(callback).Run(nullptr, SerializeError(error));
  }
}

// Resolves with an array of strings, or rejects with the error bundle.
void OnStringVecResult(ScriptMessageReplyCallback callback,
                       const std::optional<std::vector<std::string>>& result,
                       mojom::CardanoProviderErrorBundlePtr error) {
  if (result) {
    base::ListValue list;
    for (const auto& item : *result) {
      list.Append(item);
    }
    base::Value value(std::move(list));
    std::move(callback).Run(&value, nil);
  } else {
    std::move(callback).Run(nullptr, SerializeError(error));
  }
}

// Resolves with an array of UTXOs, `null` when there are none, or rejects with
// the error bundle.
void OnUtxoVecResult(ScriptMessageReplyCallback callback,
                     const std::optional<std::vector<std::string>>& result,
                     mojom::CardanoProviderErrorBundlePtr error) {
  if (error) {
    std::move(callback).Run(nullptr, SerializeError(error));
    return;
  }
  if (!result) {
    // A missing result without an error resolves to `null`.
    base::Value value;
    std::move(callback).Run(&value, nil);
    return;
  }
  base::ListValue list;
  for (const auto& item : *result) {
    list.Append(item);
  }
  base::Value value(std::move(list));
  std::move(callback).Run(&value, nil);
}

void OnNetworkIdResult(ScriptMessageReplyCallback callback,
                       int32_t network_id,
                       mojom::CardanoProviderErrorBundlePtr error) {
  if (error) {
    std::move(callback).Run(nullptr, SerializeError(error));
    return;
  }
  base::Value value(network_id);
  std::move(callback).Run(&value, nil);
}

void OnSignDataResult(ScriptMessageReplyCallback callback,
                      std::optional<base::DictValue> result,
                      mojom::CardanoProviderErrorBundlePtr error) {
  if (result) {
    base::Value value(std::move(*result));
    std::move(callback).Run(&value, nil);
  } else {
    std::move(callback).Run(nullptr, SerializeError(error));
  }
}

void OnIsEnabledResult(ScriptMessageReplyCallback callback, bool enabled) {
  base::Value value(enabled);
  std::move(callback).Run(&value, nil);
}

// Binds the `CardanoApi` remote returned by a successful `enable()` and
// resolves the page's promise so it can build its CIP-30 API object.
void OnEnableResult(base::WeakPtr<CardanoProviderTabHelper> tab_helper,
                    ScriptMessageReplyCallback callback,
                    mojo::PendingRemote<mojom::CardanoApi> api,
                    mojom::CardanoProviderErrorBundlePtr error) {
  if (error || !api || !tab_helper) {
    std::move(callback).Run(nullptr, SerializeError(error));
    return;
  }
  tab_helper->BindCardanoApi(std::move(api));
  std::move(callback).Run(nullptr, nil);
}

}  // namespace

CardanoProviderJavaScriptFeature::CardanoProviderJavaScriptFeature(
    ProfileIOS* profile)
    : JavaScriptFeature(web::ContentWorld::kPageContentWorld,
                        /*feature_scripts=*/{}),
      profile_(profile) {
  pref_change_registrar_.Init(profile->GetPrefs());
  pref_change_registrar_.Add(
      kDefaultCardanoWallet,
      base::BindRepeating(
          &CardanoProviderJavaScriptFeature::OnDefaultCardanoWalletChanged,
          base::Unretained(this)));
}

CardanoProviderJavaScriptFeature::~CardanoProviderJavaScriptFeature() = default;

// static
CardanoProviderJavaScriptFeature*
CardanoProviderJavaScriptFeature::FromBrowserState(
    web::BrowserState* browser_state) {
  DCHECK(browser_state);
  CardanoProviderJavaScriptFeature* feature =
      static_cast<CardanoProviderJavaScriptFeature*>(
          browser_state->GetUserData(kCardanoProviderJavaScriptFeatureKeyName));
  if (!feature) {
    feature = new CardanoProviderJavaScriptFeature(
        ProfileIOS::FromBrowserState(browser_state));
    browser_state->SetUserData(kCardanoProviderJavaScriptFeatureKeyName,
                               base::WrapUnique(feature));
  }
  return feature;
}

void CardanoProviderJavaScriptFeature::OnDefaultCardanoWalletChanged() {
  // Feature scripts must be explicitly updated after this pref changes.
  web::WKWebViewConfigurationProvider& config_provider =
      web::WKWebViewConfigurationProvider::FromBrowserState(profile_);
  config_provider.UpdateScripts();
}

std::vector<web::JavaScriptFeature::FeatureScript>
CardanoProviderJavaScriptFeature::GetScripts() const {
  PrefService* prefs = profile_->GetPrefs();
  if (!IsAllowed(prefs) || !IsCardanoDAppSupportEnabled() ||
      !IsDefaultCardanoWalletBrave(prefs)) {
    // Dont inject the wallet script if wallet is not enabled for this profile,
    // Cardano dApp support is disabled, or Brave is not set as the default
    // Cardano wallet provider.
    return {};
  }
  return {FeatureScript::CreateWithFilename(
      kScriptName, FeatureScript::InjectionTime::kDocumentStart,
      FeatureScript::TargetFrames::kMainFrame,
      FeatureScript::ReinjectionBehavior::kInjectOncePerWindow,
      base::BindRepeating(&web::MessageHandlerToken::GetPlaceholderReplacements,
                          base::Unretained(&token_)))};
}

bool CardanoProviderJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

std::optional<std::string>
CardanoProviderJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

void CardanoProviderJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  const url::Origin security_origin = message.security_origin();
  const web::NavigationItem* visible_item =
      web_state->GetNavigationManager()->GetVisibleItem();

  if (!message.is_main_frame() || security_origin.opaque() || !visible_item) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  const web::SSLStatus ssl = visible_item->GetSSL();
  bool displayed_mixed_content =
      (ssl.content_status & web::SSLStatus::DISPLAYED_INSECURE_CONTENT) ? true
                                                                        : false;

  // Guard against a race where an old page's JS message is delivered after a
  // navigation commit: the message origin must match the committed origin.
  const GURL committed_url = web_state->GetLastCommittedURL();
  if (!committed_url.is_valid() ||
      !security_origin.IsSameOriginWith(committed_url) ||
      displayed_mixed_content) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  std::optional<const base::Value*> validated_body =
      token_.GetValidatedScriptMessageBody(message);
  const base::DictValue* body =
      validated_body ? (*validated_body)->GetIfDict() : nullptr;
  if (!body) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  const std::string* method_string = body->FindString(kMethodKey);
  const std::string* args = body->FindString(kArgsKey);
  if (!method_string || !args) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  std::optional<ProviderMethod> method = ParseMethod(*method_string);
  if (!method) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  CardanoProviderTabHelper* tab_helper =
      CardanoProviderTabHelper::FromWebState(web_state);
  if (!tab_helper) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  std::optional<base::Value> parsed =
      base::JSONReader::Read(*args, base::JSON_PARSE_RFC);
  const base::DictValue* payload = parsed ? parsed->GetIfDict() : nullptr;
  if (!payload) {
    std::move(callback).Run(nullptr, @"Invalid args");
    return;
  }

  // `isEnabled` / `enable` are serviced by the per-origin `CardanoProvider`.
  if (*method == ProviderMethod::kIsEnabled ||
      *method == ProviderMethod::kEnable) {
    mojom::CardanoProvider* provider = tab_helper->GetProvider();
    if (!provider) {
      std::move(callback).Run(nullptr, nil);
      return;
    }
    if (*method == ProviderMethod::kIsEnabled) {
      provider->IsEnabled(
          base::BindOnce(&OnIsEnabledResult, std::move(callback)));
    } else {
      provider->Enable(base::BindOnce(&OnEnableResult, tab_helper->GetWeakPtr(),
                                      std::move(callback)));
    }
    return;
  }

  // All other (CIP-30) methods require the page to have called `enable()`
  // first, which binds the `CardanoApi` remote used here.
  mojom::CardanoApi* api = tab_helper->GetCardanoApi();
  if (!api) {
    std::move(callback).Run(
        nullptr, SerializeError(mojom::CardanoProviderErrorBundle::New(
                     kAPIErrorInternalError, "Internal error", nullptr)));
    return;
  }

  switch (*method) {
    case ProviderMethod::kIsEnabled:
    case ProviderMethod::kEnable:
      // Handled above.
      NOTREACHED();
    case ProviderMethod::kGetNetworkId:
      api->GetNetworkId(
          base::BindOnce(&OnNetworkIdResult, std::move(callback)));
      return;
    case ProviderMethod::kGetUsedAddresses:
      api->GetUsedAddresses(
          base::BindOnce(&OnStringVecResult, std::move(callback)));
      return;
    case ProviderMethod::kGetUnusedAddresses:
      api->GetUnusedAddresses(
          base::BindOnce(&OnStringVecResult, std::move(callback)));
      return;
    case ProviderMethod::kGetChangeAddress:
      api->GetChangeAddress(
          base::BindOnce(&OnStringResult, std::move(callback)));
      return;
    case ProviderMethod::kGetRewardAddresses:
      api->GetRewardAddresses(
          base::BindOnce(&OnStringVecResult, std::move(callback)));
      return;
    case ProviderMethod::kGetBalance:
      api->GetBalance(base::BindOnce(&OnStringResult, std::move(callback)));
      return;
    case ProviderMethod::kGetUtxos: {
      std::optional<std::string> amount;
      if (const std::string* amount_string = payload->FindString("amount")) {
        amount = *amount_string;
      }
      mojom::CardanoProviderPaginationPtr paginate;
      if (const base::DictValue* paginate_dict =
              payload->FindDict("paginate")) {
        std::optional<int> page = paginate_dict->FindInt("page");
        std::optional<int> limit = paginate_dict->FindInt("limit");
        if (page && limit) {
          paginate = mojom::CardanoProviderPagination::New(*page, *limit);
        }
      }
      api->GetUtxos(amount, std::move(paginate),
                    base::BindOnce(&OnUtxoVecResult, std::move(callback)));
      return;
    }
    case ProviderMethod::kSignTx: {
      const std::string* tx = payload->FindString("tx");
      if (!tx) {
        std::move(callback).Run(nullptr, @"Invalid args");
        return;
      }
      bool partial_sign = payload->FindBool("partialSign").value_or(false);
      api->SignTx(*tx, partial_sign,
                  base::BindOnce(&OnStringResult, std::move(callback)));
      return;
    }
    case ProviderMethod::kSignData: {
      const std::string* addr = payload->FindString("addr");
      const std::string* data_payload = payload->FindString("payload");
      if (!addr || !data_payload) {
        std::move(callback).Run(nullptr, @"Invalid args");
        return;
      }
      api->SignData(*addr, *data_payload,
                    base::BindOnce(&OnSignDataResult, std::move(callback)));
      return;
    }
    case ProviderMethod::kSubmitTx: {
      const std::string* tx = payload->FindString("tx");
      if (!tx) {
        std::move(callback).Run(nullptr, @"Invalid args");
        return;
      }
      api->SubmitTx(*tx, base::BindOnce(&OnStringResult, std::move(callback)));
      return;
    }
    case ProviderMethod::kGetCollateral: {
      // The page forwards CIP-30's `{ amount }` params object; accept either
      // that object or a bare amount string.
      std::string amount;
      if (const base::Value* amount_value = payload->Find("amount")) {
        if (amount_value->is_string()) {
          amount = amount_value->GetString();
        } else if (const std::string* nested =
                       amount_value->GetIfDict()
                           ? amount_value->GetDict().FindString("amount")
                           : nullptr) {
          amount = *nested;
        }
      }
      api->GetCollateral(amount,
                         base::BindOnce(&OnUtxoVecResult, std::move(callback)));
      return;
    }
  }
}

}  // namespace brave_wallet
