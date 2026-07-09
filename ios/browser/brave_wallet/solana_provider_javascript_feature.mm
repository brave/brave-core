// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_wallet/solana_provider_javascript_feature.h"

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
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_script_generated.h"
#include "brave/ios/browser/brave_wallet/solana_provider_tab_helper.h"
#include "brave/ios/web/js_messaging/message_handler_token.h"
#include "components/grit/brave_components_resources.h"
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

constexpr char kSolanaProviderJavaScriptFeatureKeyName[] =
    "solana_provider_java_script_feature";
constexpr char kScriptName[] = "solana_provider";
constexpr char kScriptHandlerName[] = "SolanaProviderMessageHandler";

constexpr char kMethodKey[] = "method";
constexpr char kArgsKey[] = "args";
constexpr char kSerializedMessageKey[] = "serializedMessage";
constexpr char kSignaturesKey[] = "signatures";
constexpr char kSendOptionsKey[] = "sendOptions";
constexpr char kPublicKeyKey[] = "publicKey";
constexpr char kSignatureKey[] = "signature";
constexpr char kSerializedTxKey[] = "serializedTx";
constexpr char kVersionKey[] = "version";
constexpr char kParamsKey[] = "params";
constexpr char kMessageKey[] = "message";
constexpr char kCodeKey[] = "code";

// The template for injecting the `@solana/web3.js` module. The module expects
// `$` and `$Object` globals to exist. It is stashed in the `_braveSolanaWeb3`
// global lexical binding so the main provider script (and the tab helper) can
// construct `solanaWeb3` objects such as `PublicKey` and `Transaction`.
//
// This must be injected on document recreation to ensure `_braveSolanaWeb3` is
// available in other scripts.
//
// Note: New line preservation is important, the shared js includes a
// sourceMappingURL comment at the end so it must be substituted on its own line
constexpr char kSolanaWeb3ScriptTemplate[] =
    "let _braveSolanaWeb3;\n"
    "(function ($, $Object, $Array){\n"
    "  if (window.isSecureContext) {\n"
    "    %s\n"
    "    _braveSolanaWeb3 = $({ solanaWeb3: $(solanaWeb3) });\n"
    "  }\n"
    "  if (typeof _braveSolanaWeb3 === 'undefined') {\n"
    "      return;\n"
    "  }\n"
    "  const freezeExceptions = $Array.of(\"BN\");\n"
    "  for (const value of $Object.values(_braveSolanaWeb3.solanaWeb3)) {\n"
    "    if (!value) {\n"
    "      continue;\n"
    "    }\n"
    "    window.__gSafeBuiltins.recursiveFreeze(value, freezeExceptions);\n"
    "  }\n"
    "  window.__gSafeBuiltins.deepFreeze(_braveSolanaWeb3.solanaWeb3)\n"
    "  window.__gSafeBuiltins.deepFreeze(_braveSolanaWeb3)\n"
    "})(window.__gSafeBuiltins.$, window.__gSafeBuiltins.$Object, "
    "window.__gSafeBuiltins.$Array)";

// The template for injecting the shared solana provider bundle which adds the
// EventEmitter methods (`on`/`off`/`emit`/...) to `window.braveSolana`. Expects
// a global `$Object` to exist.
// Note: New line preservation is important, the webpacked shared js includes a
// sourceMappingURL comment at the end so it must be substituted on its own line
constexpr char kJSProviderBundleScriptTemplate[] =
    "(function ($Object){\n"
    "  if (window.isSecureContext) {\n"
    "    %s\n"
    "  }\n"
    "})(window.__gSafeBuiltins.$Object)";

// The template for injecting the Wallet Standard library and registering it
// against `window.braveSolana` once a dApp announces it is ready. Expects `$`
// and `$Object` globals to exist.
// Note: New line preservation is important, the shared js includes a
// sourceMappingURL comment at the end so it must be substituted on its own line
constexpr char kWalletStandardScriptTemplate[] =
    "(function ($, $Object){\n"
    "  if (window.isSecureContext) {\n"
    "    %s\n"
    "    window.addEventListener('wallet-standard:app-ready', (e) => {\n"
    "      walletStandardBrave.initialize(window.braveSolana);\n"
    "    });\n"
    "  }\n"
    "})(window.__gSafeBuiltins.$, window.__gSafeBuiltins.$Object)";

// The `method` value posted by solana_provider.ts.
enum class ProviderMethod {
  kConnect,
  kDisconnect,
  kSignAndSendTransaction,
  kSignMessage,
  kRequest,
  kSignTransaction,
  kSignAllTransactions,
};

using ScriptMessageReplyCallback =
    base::OnceCallback<void(const base::Value* reply, NSString* error)>;

std::optional<ProviderMethod> ParseMethod(std::string_view method) {
  static constexpr auto kMethods =
      base::MakeFixedFlatMap<std::string_view, ProviderMethod>({
          {"connect", ProviderMethod::kConnect},
          {"disconnect", ProviderMethod::kDisconnect},
          {"signAndSendTransaction", ProviderMethod::kSignAndSendTransaction},
          {"signMessage", ProviderMethod::kSignMessage},
          {"request", ProviderMethod::kRequest},
          {"signTransaction", ProviderMethod::kSignTransaction},
          {"signAllTransactions", ProviderMethod::kSignAllTransactions},
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

bool IsDefaultWalletBrave(PrefService* prefs) {
  mojom::DefaultWallet sol_wallet = GetDefaultSolanaWallet(prefs);
  // iOS does not have a separate extension so we consider Brave the default
  // wallet for either state.
  return sol_wallet == mojom::DefaultWallet::BraveWallet ||
         sol_wallet == mojom::DefaultWallet::BraveWalletPreferExtension;
}

// Builds the JSON error string rejected to the page in the shape
// `{code: <int>, message: <string>}` expected by solana_provider.ts.
NSString* BuildErrorJson(mojom::SolanaProviderError error,
                         const std::string& error_message) {
  base::DictValue dict;
  dict.Set(kCodeKey, static_cast<int>(error));
  dict.Set(kMessageKey, error_message);
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return base::SysUTF8ToNSString(json);
}

// Builds a `SolanaSignTransactionParam` from the `{serializedMessage: [UInt8],
// signatures: [{publicKey, signature: [UInt8]}]}` object posted by the page.
mojom::SolanaSignTransactionParamPtr CreateSignTransactionParam(
    const base::Value* serialized_message,
    const base::Value* signatures) {
  const base::ListValue* serialized_message_list =
      serialized_message ? serialized_message->GetIfList() : nullptr;
  if (!serialized_message_list) {
    return nullptr;
  }
  std::vector<uint8_t> serialized_message_bytes;
  serialized_message_bytes.reserve(serialized_message_list->size());
  for (const base::Value& byte : *serialized_message_list) {
    serialized_message_bytes.push_back(
        static_cast<uint8_t>(byte.GetIfInt().value_or(0)));
  }
  std::string encoded_serialized_msg = Base58Encode(serialized_message_bytes);

  std::vector<mojom::SignaturePubkeyPairPtr> signature_pairs;
  if (const base::ListValue* signatures_list =
          signatures ? signatures->GetIfList() : nullptr) {
    for (const base::Value& signature_value : *signatures_list) {
      const base::DictValue* signature_dict = signature_value.GetIfDict();
      if (!signature_dict) {
        continue;
      }
      const std::string* public_key = signature_dict->FindString(kPublicKeyKey);
      std::vector<uint8_t> signature_bytes;
      if (const base::ListValue* bytes =
              signature_dict->FindList(kSignatureKey)) {
        signature_bytes.reserve(bytes->size());
        for (const base::Value& byte : *bytes) {
          signature_bytes.push_back(
              static_cast<uint8_t>(byte.GetIfInt().value_or(0)));
        }
      }
      signature_pairs.push_back(mojom::SignaturePubkeyPair::New(
          mojom::SolanaSignature::New(std::move(signature_bytes)),
          public_key ? *public_key : std::string()));
    }
  }
  return mojom::SolanaSignTransactionParam::New(
      std::move(encoded_serialized_msg), std::move(signature_pairs));
}

// Builds the `{serializedTx: [UInt8], version: <int>}` object the page's
// `createTransaction` helper expects from a signed transaction.
base::Value BuildSerializedTxValue(const std::vector<uint8_t>& serialized_tx,
                                   mojom::SolanaMessageVersion version) {
  base::ListValue serialized_tx_list;
  for (uint8_t byte : serialized_tx) {
    serialized_tx_list.Append(byte);
  }
  base::DictValue dict;
  dict.Set(kSerializedTxKey, std::move(serialized_tx_list));
  dict.Set(kVersionKey, static_cast<int>(version));
  return base::Value(std::move(dict));
}

}  // namespace

SolanaProviderJavaScriptFeature::SolanaProviderJavaScriptFeature(
    ProfileIOS* profile)
    : JavaScriptFeature(web::ContentWorld::kPageContentWorld,
                        /*feature_scripts=*/{}),
      profile_(profile),
      provider_bundle_js_(LoadDataResource(
          IDR_BRAVE_WALLET_SCRIPT_SOLANA_PROVIDER_SCRIPT_BUNDLE_JS)),
      solana_web3_js_(LoadDataResource(IDR_BRAVE_WALLET_SOLANA_WEB3_JS)),
      wallet_standard_js_(LoadDataResource(IDR_BRAVE_WALLET_STANDARD_JS)) {
  pref_change_registrar_.Init(profile->GetPrefs());
  pref_change_registrar_.Add(
      kDefaultSolanaWallet,
      base::BindRepeating(
          &SolanaProviderJavaScriptFeature::OnDefaultSolanaWalletChanged,
          base::Unretained(this)));
}

SolanaProviderJavaScriptFeature::~SolanaProviderJavaScriptFeature() = default;

// static
SolanaProviderJavaScriptFeature*
SolanaProviderJavaScriptFeature::FromBrowserState(
    web::BrowserState* browser_state) {
  DCHECK(browser_state);
  SolanaProviderJavaScriptFeature* feature =
      static_cast<SolanaProviderJavaScriptFeature*>(
          browser_state->GetUserData(kSolanaProviderJavaScriptFeatureKeyName));
  if (!feature) {
    feature = new SolanaProviderJavaScriptFeature(
        ProfileIOS::FromBrowserState(browser_state));
    browser_state->SetUserData(kSolanaProviderJavaScriptFeatureKeyName,
                               base::WrapUnique(feature));
  }
  return feature;
}

void SolanaProviderJavaScriptFeature::OnDefaultSolanaWalletChanged() {
  // Feature scripts must be explicitly updated after this pref changes.
  web::WKWebViewConfigurationProvider& config_provider =
      web::WKWebViewConfigurationProvider::FromBrowserState(profile_);
  config_provider.UpdateScripts();
}

std::vector<web::JavaScriptFeature::FeatureScript>
SolanaProviderJavaScriptFeature::GetScripts() const {
  PrefService* prefs = profile_->GetPrefs();
  if (!IsAllowed(prefs) || !IsDefaultWalletBrave(prefs)) {
    // Dont inject wallet scripts if wallet is not enabled for this profile or
    // Brave is not set as the default solana wallet provider
    return {};
  }
  return {
      // Inject the `@solana/web3.js` module the provider depends on first so it
      // is available in the `_braveSolanaWeb3` namespace by the time any
      // provider method is called.
      FeatureScript::CreateWithString(
          absl::StrFormat(kSolanaWeb3ScriptTemplate, solana_web3_js_),
          FeatureScript::InjectionTime::kDocumentStart,
          FeatureScript::TargetFrames::kMainFrame,
          FeatureScript::ReinjectionBehavior::kReinjectOnDocumentRecreation),
      // Inject the main provider script that installs `window.solana` /
      // `window.braveSolana`.
      FeatureScript::CreateWithFilename(
          kScriptName, FeatureScript::InjectionTime::kDocumentStart,
          FeatureScript::TargetFrames::kMainFrame,
          FeatureScript::ReinjectionBehavior::kInjectOncePerWindow,
          base::BindRepeating(
              &web::MessageHandlerToken::GetPlaceholderReplacements,
              base::Unretained(&token_))),
      // Inject the shared provider bundle that adds the EventEmitter methods to
      // the provider.
      FeatureScript::CreateWithString(
          absl::StrFormat(kJSProviderBundleScriptTemplate, provider_bundle_js_),
          FeatureScript::InjectionTime::kDocumentStart,
          FeatureScript::TargetFrames::kMainFrame),
      // Inject the Wallet Standard library.
      FeatureScript::CreateWithString(
          absl::StrFormat(kWalletStandardScriptTemplate, wallet_standard_js_),
          FeatureScript::InjectionTime::kDocumentStart,
          FeatureScript::TargetFrames::kMainFrame)};
}

bool SolanaProviderJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

std::optional<std::string>
SolanaProviderJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

void SolanaProviderJavaScriptFeature::ScriptMessageReceivedWithReply(
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
  if (!method_string) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  std::optional<ProviderMethod> method = ParseMethod(*method_string);
  if (!method) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  // `args` is a JSON string of the method payload. It may be absent for methods
  // that take no arguments (e.g. `connect()` / `disconnect()`), in which case
  // `JSON.stringify(undefined)` yields `undefined` and the field is omitted.
  const std::string* args = body->FindString(kArgsKey);

  SolanaProviderTabHelper* tab_helper =
      SolanaProviderTabHelper::FromWebState(web_state);
  mojom::SolanaProvider* provider =
      tab_helper ? tab_helper->GetProvider() : nullptr;
  if (!provider) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  // Parse `args` into a base::Value once. All methods expect either an object
  // or a list depending on the method.
  std::optional<base::Value> parsed_args =
      args ? base::JSONReader::Read(*args, base::JSON_PARSE_RFC) : std::nullopt;

  base::WeakPtr<SolanaProviderTabHelper> weak_tab_helper =
      tab_helper->GetWeakPtr();

  switch (*method) {
    case ProviderMethod::kConnect: {
      std::optional<base::DictValue> arg;
      if (parsed_args && parsed_args->is_dict()) {
        arg = std::move(*parsed_args).TakeDict();
      }
      provider->Connect(
          std::move(arg),
          base::BindOnce(
              [](base::WeakPtr<SolanaProviderTabHelper> tab_helper,
                 ScriptMessageReplyCallback callback,
                 mojom::SolanaProviderError error,
                 const std::string& error_message,
                 const std::string& public_key) {
                if (error != mojom::SolanaProviderError::kSuccess) {
                  std::move(callback).Run(nullptr,
                                          BuildErrorJson(error, error_message));
                  return;
                }
                base::Value reply(public_key);
                std::move(callback).Run(&reply, nil);
                if (tab_helper) {
                  tab_helper->EmitConnectEvent(public_key);
                  tab_helper->UpdateSolanaProperties();
                }
              },
              weak_tab_helper, std::move(callback)));
      return;
    }
    case ProviderMethod::kDisconnect: {
      provider->Disconnect();
      base::Value reply((base::DictValue()));
      std::move(callback).Run(&reply, nil);
      return;
    }
    case ProviderMethod::kSignAndSendTransaction: {
      base::DictValue* dict = parsed_args ? parsed_args->GetIfDict() : nullptr;
      mojom::SolanaSignTransactionParamPtr param =
          dict ? CreateSignTransactionParam(dict->Find(kSerializedMessageKey),
                                            dict->Find(kSignaturesKey))
               : nullptr;
      if (!param) {
        std::move(callback).Run(nullptr, @"Invalid args");
        return;
      }
      std::optional<base::DictValue> send_options;
      if (base::DictValue* options = dict->FindDict(kSendOptionsKey)) {
        send_options = std::move(*options);
      }
      provider->SignAndSendTransaction(
          std::move(param), std::move(send_options),
          base::BindOnce(
              [](ScriptMessageReplyCallback callback,
                 mojom::SolanaProviderError error,
                 const std::string& error_message, base::DictValue result) {
                if (error != mojom::SolanaProviderError::kSuccess) {
                  std::move(callback).Run(nullptr,
                                          BuildErrorJson(error, error_message));
                  return;
                }
                base::Value reply(std::move(result));
                std::move(callback).Run(&reply, nil);
              },
              std::move(callback)));
      return;
    }
    case ProviderMethod::kSignMessage: {
      base::ListValue* list = parsed_args ? parsed_args->GetIfList() : nullptr;
      const base::ListValue* blob =
          list && !list->empty() ? (*list)[0].GetIfList() : nullptr;
      if (!blob) {
        std::move(callback).Run(nullptr, @"Invalid args");
        return;
      }
      std::vector<uint8_t> blob_msg;
      blob_msg.reserve(blob->size());
      for (const base::Value& byte : *blob) {
        blob_msg.push_back(static_cast<uint8_t>(byte.GetIfInt().value_or(0)));
      }
      std::optional<std::string> display_encoding;
      if (list->size() > 1 && (*list)[1].is_string()) {
        display_encoding = (*list)[1].GetString();
      }
      provider->SignMessage(
          blob_msg, display_encoding,
          base::BindOnce(
              [](ScriptMessageReplyCallback callback,
                 mojom::SolanaProviderError error,
                 const std::string& error_message, base::DictValue result) {
                if (error != mojom::SolanaProviderError::kSuccess) {
                  std::move(callback).Run(nullptr,
                                          BuildErrorJson(error, error_message));
                  return;
                }
                // `result` is `{publicKey: <base58>, signature: <base58>}`.
                // Convert the signature to a byte list so the page can build a
                // `Uint8Array` from it.
                const std::string* public_key =
                    result.FindString(kPublicKeyKey);
                const std::string* signature = result.FindString(kSignatureKey);
                std::vector<uint8_t> signature_bytes(kSolanaSignatureSize);
                if (!public_key || !signature ||
                    !Base58Decode(*signature, &signature_bytes,
                                  signature_bytes.size())) {
                  std::move(callback).Run(
                      nullptr,
                      BuildErrorJson(mojom::SolanaProviderError::kInternalError,
                                     error_message));
                  return;
                }
                base::ListValue signature_list;
                for (uint8_t byte : signature_bytes) {
                  signature_list.Append(byte);
                }
                base::DictValue reply_dict;
                reply_dict.Set(kPublicKeyKey, *public_key);
                reply_dict.Set(kSignatureKey, std::move(signature_list));
                base::Value reply(std::move(reply_dict));
                std::move(callback).Run(&reply, nil);
              },
              std::move(callback)));
      return;
    }
    case ProviderMethod::kRequest: {
      base::DictValue* arg = parsed_args ? parsed_args->GetIfDict() : nullptr;
      const std::string* request_method =
          arg ? arg->FindString(kMethodKey) : nullptr;
      if (!request_method) {
        std::move(callback).Run(
            nullptr, BuildErrorJson(mojom::SolanaProviderError::kInvalidParams,
                                    "Invalid args"));
        return;
      }
      // `signMessage` requests carry the message as a `[UInt8]` list under
      // `params.message`. Convert it to a binary value so it maps to the mojo
      // blob argument.
      if (*request_method == "signMessage") {
        if (base::DictValue* params = arg->FindDict(kParamsKey)) {
          if (const base::ListValue* blob = params->FindList(kMessageKey)) {
            std::vector<uint8_t> blob_msg;
            blob_msg.reserve(blob->size());
            for (const base::Value& byte : *blob) {
              blob_msg.push_back(
                  static_cast<uint8_t>(byte.GetIfInt().value_or(0)));
            }
            params->Set(kMessageKey, base::Value(std::move(blob_msg)));
          }
        }
      }
      std::string method_copy = *request_method;
      provider->Request(
          std::move(*arg),
          base::BindOnce(
              [](base::WeakPtr<SolanaProviderTabHelper> tab_helper,
                 ScriptMessageReplyCallback callback,
                 const std::string& request_method,
                 mojom::SolanaProviderError error,
                 const std::string& error_message, base::DictValue result) {
                if (error != mojom::SolanaProviderError::kSuccess) {
                  std::move(callback).Run(nullptr,
                                          BuildErrorJson(error, error_message));
                  return;
                }
                if (request_method == "connect") {
                  const std::string* public_key =
                      result.FindString(kPublicKeyKey);
                  base::Value reply(public_key ? *public_key : std::string());
                  std::move(callback).Run(&reply, nil);
                  if (tab_helper && public_key) {
                    tab_helper->EmitConnectEvent(*public_key);
                    tab_helper->UpdateSolanaProperties();
                  }
                  return;
                }
                base::Value reply(std::move(result));
                std::move(callback).Run(&reply, nil);
                if (request_method == "disconnect" && tab_helper) {
                  tab_helper->EmitDisconnectEvent();
                }
              },
              weak_tab_helper, std::move(callback), method_copy));
      return;
    }
    case ProviderMethod::kSignTransaction: {
      base::DictValue* dict = parsed_args ? parsed_args->GetIfDict() : nullptr;
      mojom::SolanaSignTransactionParamPtr param =
          dict ? CreateSignTransactionParam(dict->Find(kSerializedMessageKey),
                                            dict->Find(kSignaturesKey))
               : nullptr;
      if (!param) {
        std::move(callback).Run(nullptr, @"Invalid args");
        return;
      }
      provider->SignTransaction(
          std::move(param),
          base::BindOnce(
              [](ScriptMessageReplyCallback callback,
                 mojom::SolanaProviderError error,
                 const std::string& error_message,
                 const std::vector<uint8_t>& serialized_tx,
                 mojom::SolanaMessageVersion version) {
                if (error != mojom::SolanaProviderError::kSuccess) {
                  std::move(callback).Run(nullptr,
                                          BuildErrorJson(error, error_message));
                  return;
                }
                base::Value reply =
                    BuildSerializedTxValue(serialized_tx, version);
                std::move(callback).Run(&reply, nil);
              },
              std::move(callback)));
      return;
    }
    case ProviderMethod::kSignAllTransactions: {
      base::ListValue* transactions =
          parsed_args ? parsed_args->GetIfList() : nullptr;
      std::vector<mojom::SolanaSignTransactionParamPtr> params;
      if (transactions) {
        for (const base::Value& transaction : *transactions) {
          const base::DictValue* dict = transaction.GetIfDict();
          mojom::SolanaSignTransactionParamPtr param =
              dict ? CreateSignTransactionParam(
                         dict->Find(kSerializedMessageKey),
                         dict->Find(kSignaturesKey))
                   : nullptr;
          if (param) {
            params.push_back(std::move(param));
          }
        }
      }
      if (params.empty()) {
        std::move(callback).Run(nullptr, @"Invalid args");
        return;
      }
      provider->SignAllTransactions(
          std::move(params),
          base::BindOnce(
              [](ScriptMessageReplyCallback callback,
                 mojom::SolanaProviderError error,
                 const std::string& error_message,
                 const std::vector<std::vector<uint8_t>>& serialized_txs,
                 const std::vector<mojom::SolanaMessageVersion>& versions) {
                if (error != mojom::SolanaProviderError::kSuccess ||
                    serialized_txs.size() != versions.size()) {
                  std::move(callback).Run(nullptr,
                                          BuildErrorJson(error, error_message));
                  return;
                }
                base::ListValue reply_list;
                for (size_t i = 0; i < serialized_txs.size(); ++i) {
                  reply_list.Append(
                      BuildSerializedTxValue(serialized_txs[i], versions[i]));
                }
                base::Value reply(std::move(reply_list));
                std::move(callback).Run(&reply, nil);
              },
              std::move(callback)));
      return;
    }
  }
}

}  // namespace brave_wallet
