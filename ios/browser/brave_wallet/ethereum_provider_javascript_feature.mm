// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_wallet/ethereum_provider_javascript_feature.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_script_generated.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_utils.h"
#include "brave/ios/browser/brave_wallet/ethereum_provider_tab_helper.h"
#include "brave/ios/web/js_messaging/message_handler_token.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/navigation/navigation_item.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/security/ssl_status.h"
#include "ios/web/public/web_state.h"
#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

constexpr char kEthereumProviderJavaScriptFeatureKeyName[] =
    "ethereum_provider_java_script_feature";
constexpr char kScriptName[] = "ethereum_provider";
constexpr char kScriptHandlerName[] = "EthereumProviderMessageHandler";

constexpr char kMethodKey[] = "method";
constexpr char kArgsKey[] = "args";
constexpr char kPayloadMethodKey[] = "method";
constexpr char kPayloadParamsKey[] = "params";

// The template for injecting the shared ethereum provider bundle which expects
// a global $Object to exist. Expects `window.__gSafeBuiltins` to exist.
// Note: New line preservation is important, the webpacked shared js includes a
// sourceMappingURL comment at the end so it must be substituted on its own line
constexpr char kJSProviderBundleScriptTemplate[] =
    "(function ($Object){\n"
    "  if (window.isSecureContext) {\n"
    "    %s\n"
    "  }\n"
    "})(window.__gSafeBuiltins.$Object)";

// Methods that `window.ethereum.send` may be called with as a single string
// argument (with no params).
constexpr auto kSupportedSingleArgMethods =
    base::MakeFixedFlatSet<std::string_view>(
        {"net_listening", "net_peerCount", "net_version", "eth_chainId",
         "eth_syncing", "eth_coinbase", "eth_mining", "eth_hashrate",
         "eth_accounts", "eth_newBlockFilter",
         "eth_newPendingTransactionFilter"});

// The `method` value posted by ethereum_provider.ts.
enum class ProviderMethod {
  kRequest,
  kIsConnected,
  kEnable,
  kSend,
  kSendAsync,
  kIsUnlocked,
};

using ScriptMessageReplyCallback =
    base::OnceCallback<void(const base::Value* reply, NSString* error)>;

// Forwards an `EthereumProviderResponse` from the provider back to the page's
// pending promise.
void OnProviderResponse(base::WeakPtr<EthereumProviderTabHelper> tab_helper,
                        ScriptMessageReplyCallback callback,
                        mojom::EthereumProviderResponsePtr response) {
  // Refresh the injected `chainId` / `networkVersion` / `selectedAddress`
  // values on the page when the provider indicates they may have changed.
  if (response->update_bind_js_properties && tab_helper) {
    tab_helper->UpdateEthereumProperties();
  }

  if (response->reject) {
    // The page rejects with the JSON-serialized error; ethereum_provider.ts
    // strips the `Error: ` prefix and parses it.
    std::string json;
    base::JSONWriter::Write(response->formed_response, &json);
    std::move(callback).Run(nullptr, base::SysUTF8ToNSString(json));
    return;
  }
  std::move(callback).Run(&response->formed_response, nil);
}

std::optional<ProviderMethod> ParseMethod(std::string_view method) {
  static constexpr auto kMethods =
      base::MakeFixedFlatMap<std::string_view, ProviderMethod>({
          {"request", ProviderMethod::kRequest},
          {"isConnected", ProviderMethod::kIsConnected},
          {"enable", ProviderMethod::kEnable},
          {"send", ProviderMethod::kSend},
          {"sendAsync", ProviderMethod::kSendAsync},
          {"isUnlocked", ProviderMethod::kIsUnlocked},
      });
  auto it = kMethods.find(method);
  if (it == kMethods.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::string LoadDataResource(int id) {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    return resource_bundle.LoadDataResourceString(id);
  }
  return std::string(resource_bundle.GetRawDataResource(id));
}

}  // namespace

EthereumProviderJavaScriptFeature::EthereumProviderJavaScriptFeature(
    ProfileIOS* profile)
    : JavaScriptFeature(web::ContentWorld::kPageContentWorld,
                        /*feature_scripts=*/{}),
      profile_(profile),
      provider_bundle_js_(LoadDataResource(
          IDR_BRAVE_WALLET_SCRIPT_ETHEREUM_PROVIDER_SCRIPT_BUNDLE_JS)) {
  pref_change_registrar_.Init(profile->GetPrefs());
  pref_change_registrar_.Add(
      kDefaultEthereumWallet,
      base::BindRepeating(
          &EthereumProviderJavaScriptFeature::OnDefaultEthereumWalletChanged,
          base::Unretained(this)));
}

EthereumProviderJavaScriptFeature::~EthereumProviderJavaScriptFeature() =
    default;

// static
EthereumProviderJavaScriptFeature*
EthereumProviderJavaScriptFeature::FromBrowserState(
    web::BrowserState* browser_state) {
  DCHECK(browser_state);
  EthereumProviderJavaScriptFeature* feature =
      static_cast<EthereumProviderJavaScriptFeature*>(
          browser_state->GetUserData(
              kEthereumProviderJavaScriptFeatureKeyName));
  if (!feature) {
    feature = new EthereumProviderJavaScriptFeature(
        ProfileIOS::FromBrowserState(browser_state));
    browser_state->SetUserData(kEthereumProviderJavaScriptFeatureKeyName,
                               base::WrapUnique(feature));
  }
  return feature;
}

void EthereumProviderJavaScriptFeature::OnDefaultEthereumWalletChanged() {
  // Feature scripts must be explicitly updated after this pref changes.
  web::WKWebViewConfigurationProvider& config_provider =
      web::WKWebViewConfigurationProvider::FromBrowserState(profile_);
  config_provider.UpdateScripts();
}

std::vector<web::JavaScriptFeature::FeatureScript>
EthereumProviderJavaScriptFeature::GetScripts() const {
  PrefService* prefs = profile_->GetPrefs();
  if (!IsAllowed(prefs) || !IsDefaultEthereumWalletBrave(prefs)) {
    // Dont inject wallet scripts if wallet is not enabled for this profile or
    // Brave is not set as the default ethereum wallet provider
    return {};
  }
  return {
      FeatureScript::CreateWithFilename(
          kScriptName, FeatureScript::InjectionTime::kDocumentStart,
          FeatureScript::TargetFrames::kMainFrame,
          FeatureScript::ReinjectionBehavior::kInjectOncePerWindow,
          base::BindRepeating(
              &web::MessageHandlerToken::GetPlaceholderReplacements,
              base::Unretained(&token_))),
      FeatureScript::CreateWithString(
          absl::StrFormat(kJSProviderBundleScriptTemplate, provider_bundle_js_),
          FeatureScript::InjectionTime::kDocumentStart,
          FeatureScript::TargetFrames::kMainFrame)};
}

bool EthereumProviderJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

std::optional<std::string>
EthereumProviderJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

void EthereumProviderJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  const url::Origin security_origin = message.security_origin();

  if (!message.is_main_frame() || security_origin.opaque()) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  const web::SSLStatus ssl =
      web_state->GetNavigationManager()->GetVisibleItem()->GetSSL();
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

  EthereumProviderTabHelper* tab_helper =
      EthereumProviderTabHelper::FromWebState(web_state);
  mojom::EthereumProvider* provider =
      tab_helper ? tab_helper->GetProvider() : nullptr;
  if (!provider) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  switch (*method) {
    case ProviderMethod::kRequest: {
      std::optional<base::Value> request_payload =
          base::JSONReader::Read(*args, base::JSON_PARSE_RFC);
      if (!request_payload) {
        std::move(callback).Run(nullptr, @"Invalid args");
        return;
      }
      provider->Request(
          std::move(*request_payload),
          base::BindOnce(&OnProviderResponse, tab_helper->GetWeakPtr(),
                         std::move(callback)));
      return;
    }
    case ProviderMethod::kIsConnected:
      // The page's `isConnected()` resolves locally
      std::move(callback).Run(nullptr, nil);
      return;
    case ProviderMethod::kEnable:
      provider->Enable(base::BindOnce(
          &OnProviderResponse, tab_helper->GetWeakPtr(), std::move(callback)));
      return;
    case ProviderMethod::kSendAsync: {
      std::optional<base::Value> request_payload =
          base::JSONReader::Read(*args, base::JSON_PARSE_RFC);
      if (!request_payload) {
        std::move(callback).Run(nullptr, @"Invalid args");
        return;
      }
      provider->SendAsync(
          std::move(*request_payload),
          base::BindOnce(&OnProviderResponse, tab_helper->GetWeakPtr(),
                         std::move(callback)));
      return;
    }
    case ProviderMethod::kSend: {
      // `args` is a JSON string of `{ method: string, params: unknown }`.
      std::optional<base::Value> parsed =
          base::JSONReader::Read(*args, base::JSON_PARSE_RFC);
      base::DictValue* payload = parsed ? parsed->GetIfDict() : nullptr;
      const std::string* send_method =
          payload ? payload->FindString(kPayloadMethodKey) : nullptr;
      if (!send_method) {
        std::move(callback).Run(nullptr, @"Invalid args");
        return;
      }
      base::Value* params = payload->Find(kPayloadParamsKey);
      const bool has_params = params && !params->is_none();

      if (send_method->empty()) {
        if (has_params) {
          provider->SendAsync(
              std::move(*params),
              base::BindOnce(&OnProviderResponse, tab_helper->GetWeakPtr(),
                             std::move(callback)));
        } else {
          // Empty method with no params is not valid.
          std::move(callback).Run(nullptr, @"Invalid args");
        }
        return;
      }

      if (!kSupportedSingleArgMethods.contains(*send_method) && !has_params) {
        // If it is not a single-arg supported method and there are no
        // parameters then it is not a valid call.
        std::move(callback).Run(nullptr, @"Invalid args");
        return;
      }

      base::ListValue params_list;
      if (base::ListValue* list = params ? params->GetIfList() : nullptr) {
        params_list = std::move(*list);
      }
      provider->Send(
          *send_method, std::move(params_list),
          base::BindOnce(&OnProviderResponse, tab_helper->GetWeakPtr(),
                         std::move(callback)));
      return;
    }
    case ProviderMethod::kIsUnlocked:
      provider->IsLocked(base::BindOnce(
          [](ScriptMessageReplyCallback callback, bool is_locked) {
            base::Value value(!is_locked);
            std::move(callback).Run(&value, nil);
          },
          std::move(callback)));
      return;
  }
}

}  // namespace brave_wallet
