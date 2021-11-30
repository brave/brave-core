/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_js_handler.h"

#include <limits>
#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "brave/components/brave_wallet/renderer/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_script_generated.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

// By default we allow extensions to overwrite the window.ethereum object
// but if the user goes into settings and explicitly selects to use Brave Wallet
// then we will block modifications to window.ethereum here.
const char kEthereumNonWritable[] =
    R"(;(function() {
           Object.defineProperty(window, 'ethereum', {
             value: window.ethereum,
             writable: false
           });
    })();)";

static base::NoDestructor<std::string> g_provider_script("");

std::string LoadDataResource(const int id) {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    return resource_bundle.LoadDataResourceString(id);
  }

  return std::string(resource_bundle.GetRawDataResource(id));
}

v8::MaybeLocal<v8::Value> GetProperty(v8::Local<v8::Context> context,
                                      v8::Local<v8::Value> object,
                                      const std::u16string& name) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::String> name_str =
      gin::ConvertToV8(isolate, name).As<v8::String>();
  v8::Local<v8::Object> object_obj;
  if (!object->ToObject(context).ToLocal(&object_obj)) {
    return v8::MaybeLocal<v8::Value>();
  }

  return object_obj->Get(context, name_str);
}

void CallMethodOfObject(blink::WebLocalFrame* web_frame,
                        const std::u16string& object_name,
                        const std::u16string& method_name,
                        base::Value arguments) {
  if (web_frame->IsProvisional())
    return;
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  v8::Local<v8::Context> context = web_frame->MainWorldScriptContext();
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(v8::Isolate::GetCurrent(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Value> object;
  v8::Local<v8::Value> method;
  if (!GetProperty(context, context->Global(), object_name).ToLocal(&object) ||
      !GetProperty(context, object, method_name).ToLocal(&method)) {
    return;
  }

  // Without the IsFunction test here JS blocking from content settings
  // will trigger a DCHECK crash.
  if (method.IsEmpty() || !method->IsFunction()) {
    return;
  }

  std::vector<v8::Local<v8::Value>> args;
  for (auto const& argument : arguments.GetList()) {
    args.push_back(
        content::V8ValueConverter::Create()->ToV8Value(&argument, context));
  }

  web_frame->ExecuteMethodAndReturnValue(v8::Local<v8::Function>::Cast(method),
                                         object, static_cast<int>(args.size()),
                                         args.data());
}

std::unique_ptr<base::Value> GetJsonRpcRequest(
    const std::string& method,
    std::unique_ptr<base::Value> params) {
  auto dictionary =
      base::Value::ToUniquePtrValue(base::Value(base::Value::Type::DICTIONARY));
  dictionary->SetKey("jsonrpc", base::Value("2.0"));
  dictionary->SetKey("method", base::Value(method));
  dictionary->SetKey("params", params->Clone());
  dictionary->SetKey("id", base::Value("1"));
  return dictionary;
}

}  // namespace

namespace brave_wallet {

void BraveWalletJSHandler::OnRequestPermissionsAccountsRequested(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    const std::vector<std::string>& accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  first_allowed_account_.clear();
  if (accounts.size() > 0)
    first_allowed_account_ = accounts[0];
  // Note that we'll update this from `AccountsChangedEvent` as well, but we
  // need to update it earlier here before we give a response in case the JS
  // page has a handler that checks window.ethereum.selectedAddress
  UpdateAndBindJSProperties();
  std::unique_ptr<base::Value> formed_response;
  bool success = error == mojom::ProviderError::kSuccess;
  if (success && accounts.empty()) {
    formed_response =
        GetProviderErrorDictionary(mojom::ProviderError::kUserRejectedRequest,
                                   "User rejected the request.");
  } else if (!success) {
    formed_response = GetProviderErrorDictionary(error, error_message);
  } else {
    formed_response =
        base::Value::ToUniquePtrValue(PermissionRequestResponseToValue(
            url::Origin(render_frame_->GetWebFrame()->GetSecurityOrigin()),
            accounts));
  }

  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response),
               success && !accounts.empty());
}

void BraveWalletJSHandler::OnEthereumPermissionRequested(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    const std::vector<std::string>& accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  first_allowed_account_.clear();
  if (accounts.size() > 0)
    first_allowed_account_ = accounts[0];
  // Note that we'll update this from `AccountsChangedEvent` as well, but we
  // need to update it earlier here before we give a response in case the JS
  // page has a handler that checks window.ethereum.selectedAddress
  UpdateAndBindJSProperties();
  std::unique_ptr<base::Value> formed_response;
  bool success = error == mojom::ProviderError::kSuccess;
  if (success && accounts.empty()) {
    formed_response =
        GetProviderErrorDictionary(mojom::ProviderError::kUserRejectedRequest,
                                   "User rejected the request.");
  } else if (!success) {
    formed_response = GetProviderErrorDictionary(error, error_message);
  } else {
    formed_response = base::Value::ToUniquePtrValue(base::ListValue());
    for (size_t i = 0; i < accounts.size(); i++) {
      formed_response->Append(base::Value(accounts[i]));
    }
  }

  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response),
               success && !accounts.empty());
}

void BraveWalletJSHandler::OnIsUnlocked(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool locked) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  base::Value result = base::Value(!locked);
  v8::Local<v8::Value> local_result =
      content::V8ValueConverter::Create()->ToV8Value(&result, context);
  ALLOW_UNUSED_LOCAL(resolver->Resolve(context, local_result));
}

void BraveWalletJSHandler::OnGetAllowedAccounts(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    const std::vector<std::string>& accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  std::unique_ptr<base::Value> formed_response;
  if (error != mojom::ProviderError::kSuccess) {
    formed_response = GetProviderErrorDictionary(error, error_message);
  } else {
    formed_response = base::Value::ToUniquePtrValue(base::ListValue());
    for (size_t i = 0; i < accounts.size(); i++) {
      formed_response->Append(base::Value(accounts[i]));
    }
  }

  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response),
               error == mojom::ProviderError::kSuccess);
}

void BraveWalletJSHandler::OnGetGetPermissionsAccountsRequested(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    const std::vector<std::string>& accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  first_allowed_account_.clear();
  if (accounts.size() > 0)
    first_allowed_account_ = accounts[0];
  // Note that we'll update this from `AccountsChangedEvent` as well, but we
  // need to update it earlier here before we give a response in case the JS
  // page has a handler that checks window.ethereum.selectedAddress
  UpdateAndBindJSProperties();
  std::unique_ptr<base::Value> formed_response;
  if (error == mojom::ProviderError::kSuccess) {
    formed_response =
        base::Value::ToUniquePtrValue(PermissionRequestResponseToValue(
            url::Origin(render_frame_->GetWebFrame()->GetSecurityOrigin()),
            accounts));
  } else {
    formed_response = GetProviderErrorDictionary(error, error_message);
  }

  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response),
               error == mojom::ProviderError::kSuccess);
}

void BraveWalletJSHandler::OnAddOrSwitchEthereumChain(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    mojom::ProviderError error,
    const std::string& error_message) {
  std::unique_ptr<base::Value> formed_response;
  if (error == mojom::ProviderError::kSuccess) {
    formed_response = base::Value::ToUniquePtrValue(base::Value());
  } else {
    formed_response = GetProviderErrorDictionary(error, error_message);
  }
  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response),
               error == mojom::ProviderError::kSuccess);
}

void BraveWalletJSHandler::OnAddAndApproveTransaction(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    const std::string& tx_hash,
    mojom::ProviderError error,
    const std::string& error_message) {
  std::unique_ptr<base::Value> formed_response;
  if (error == mojom::ProviderError::kSuccess) {
    formed_response = base::Value::ToUniquePtrValue(base::Value(tx_hash));
  } else {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams, error_message);
  }

  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response),
               error == mojom::ProviderError::kSuccess);
}

void BraveWalletJSHandler::OnSignRecoverMessage(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    const std::string& signature,
    mojom::ProviderError error,
    const std::string& error_message) {
  std::unique_ptr<base::Value> formed_response;
  if (error == mojom::ProviderError::kSuccess) {
    formed_response = base::Value::ToUniquePtrValue(base::Value(signature));
  } else {
    formed_response = GetProviderErrorDictionary(error, error_message);
  }

  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response),
               error == mojom::ProviderError::kSuccess);
}

void BraveWalletJSHandler::OnAddSuggestToken(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    bool accepted,
    mojom::ProviderError error,
    const std::string& error_message) {
  std::unique_ptr<base::Value> formed_response;
  if (error == mojom::ProviderError::kSuccess) {
    formed_response = base::Value::ToUniquePtrValue(base::Value(accepted));
  } else {
    formed_response = GetProviderErrorDictionary(error, error_message);
  }

  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response),
               error == mojom::ProviderError::kSuccess);
}

void BraveWalletJSHandler::SendResponse(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    std::unique_ptr<base::Value> formed_response,
    bool success) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);

  if (global_callback || force_json_response) {
    auto full_formed_response = brave_wallet::ToProviderResponse(
        std::move(id), success ? formed_response.get() : nullptr,
        success ? nullptr : formed_response.get());
    formed_response = std::move(full_formed_response);
  }

  v8::Local<v8::Value> result = content::V8ValueConverter::Create()->ToV8Value(
      formed_response.get(), context);
  if (global_callback) {
    v8::Local<v8::Value> result_null = v8::Null(isolate);
    v8::Local<v8::Value> argv[] = {success ? result_null : result,
                                   success ? result : result_null};
    v8::Local<v8::Function> callback_local =
        v8::Local<v8::Function>::New(isolate, *global_callback);
    render_frame_->GetWebFrame()->CallFunctionEvenIfScriptDisabled(
        callback_local, v8::Object::New(isolate), 2, argv);
    return;
  }

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  if (success) {
    ALLOW_UNUSED_LOCAL(resolver->Resolve(context, result));
  } else {
    ALLOW_UNUSED_LOCAL(resolver->Reject(context, result));
  }
}

BraveWalletJSHandler::BraveWalletJSHandler(content::RenderFrame* render_frame,
                                           bool brave_use_native_wallet,
                                           bool allow_overwrite_window_ethereum)
    : render_frame_(render_frame),
      brave_use_native_wallet_(brave_use_native_wallet),
      allow_overwrite_window_ethereum_(allow_overwrite_window_ethereum),
      is_connected_(false) {
  if (g_provider_script->empty()) {
    *g_provider_script =
        LoadDataResource(IDR_BRAVE_WALLET_SCRIPT_BRAVE_WALLET_SCRIPT_BUNDLE_JS);
  }
  EnsureConnected();
}

BraveWalletJSHandler::~BraveWalletJSHandler() = default;

bool BraveWalletJSHandler::EnsureConnected() {
  if (brave_use_native_wallet_ && !brave_wallet_provider_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        brave_wallet_provider_.BindNewPipeAndPassReceiver());
    brave_wallet_provider_->Init(receiver_.BindNewPipeAndPassRemote());
    brave_wallet_provider_.set_disconnect_handler(
        base::BindOnce(&BraveWalletJSHandler::OnRemoteDisconnect,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  return brave_wallet_provider_.is_bound();
}

void BraveWalletJSHandler::OnRemoteDisconnect() {
  brave_wallet_provider_.reset();
  receiver_.reset();
  EnsureConnected();
}

void BraveWalletJSHandler::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  CreateEthereumObject(isolate, context);
  InjectInitScript();
}

void BraveWalletJSHandler::CreateEthereumObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> ethereum_obj;
  v8::Local<v8::Object> metamask_obj;
  v8::Local<v8::Value> ethereum_value;
  if (!global->Get(context, gin::StringToV8(isolate, "ethereum"))
           .ToLocal(&ethereum_value) ||
      !ethereum_value->IsObject()) {
    ethereum_obj = v8::Object::New(isolate);
    metamask_obj = v8::Object::New(isolate);
    global->Set(context, gin::StringToSymbol(isolate, "ethereum"), ethereum_obj)
        .Check();
    ethereum_obj
        ->Set(context, gin::StringToSymbol(isolate, "_metamask"), metamask_obj)
        .Check();
    BindFunctionsToObject(isolate, context, ethereum_obj, metamask_obj);
    UpdateAndBindJSProperties(isolate, context, ethereum_obj);
  } else {
    render_frame_->GetWebFrame()->AddMessageToConsole(
        blink::WebConsoleMessage(blink::mojom::ConsoleMessageLevel::kWarning,
                                 "Brave Wallet will not insert window.ethereum "
                                 "because it already exists!"));
  }
}

void BraveWalletJSHandler::UpdateAndBindJSProperties() {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  v8::Local<v8::Context> context = web_frame->MainWorldScriptContext();
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(v8::Isolate::GetCurrent(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Value> ethereum_value;
  v8::Local<v8::Object> ethereum_obj;
  if (!GetProperty(context, context->Global(), u"ethereum")
           .ToLocal(&ethereum_value))
    return;
  if (ethereum_value->ToObject(context).ToLocal(&ethereum_obj)) {
    UpdateAndBindJSProperties(v8::Isolate::GetCurrent(), context, ethereum_obj);
  }
}

void BraveWalletJSHandler::UpdateAndBindJSProperties(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Object> ethereum_obj) {
  v8::Local<v8::Primitive> undefined(v8::Undefined(isolate));
  ethereum_obj
      ->Set(context, gin::StringToSymbol(isolate, "chainId"),
            gin::StringToV8(isolate, chain_id_))
      .Check();

  // We have no easy way to convert a uin256 to a decimal number string yet
  // and this is a deprecated property so it's not very important.
  // So we only include it when the chain ID is <= uint64_t max value.
  uint256_t chain_id_uint256;
  if (HexValueToUint256(chain_id_, &chain_id_uint256) &&
      chain_id_uint256 <= (uint256_t)std::numeric_limits<uint64_t>::max()) {
    uint64_t networkVersion = (uint64_t)chain_id_uint256;
    ethereum_obj
        ->Set(context, gin::StringToSymbol(isolate, "networkVersion"),
              gin::StringToV8(isolate, std::to_string(networkVersion)))
        .Check();
  } else {
    ethereum_obj
        ->Set(context, gin::StringToSymbol(isolate, "networkVersion"),
              undefined)
        .Check();
  }

  // Note this does not return the selected account, but it returns the
  // first connected account that was given permissions.
  if (first_allowed_account_.empty()) {
    ethereum_obj
        ->Set(context, gin::StringToSymbol(isolate, "selectedAddress"),
              undefined)
        .Check();
  } else {
    ethereum_obj
        ->Set(context, gin::StringToSymbol(isolate, "selectedAddress"),
              gin::StringToV8(isolate, first_allowed_account_))
        .Check();
  }
}

void BraveWalletJSHandler::BindFunctionsToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Object> ethereum_object,
    v8::Local<v8::Object> metamask_object) {
  BindFunctionToObject(isolate, ethereum_object, "request",
                       base::BindRepeating(&BraveWalletJSHandler::Request,
                                           base::Unretained(this), isolate));
  BindFunctionToObject(isolate, ethereum_object, "isConnected",
                       base::BindRepeating(&BraveWalletJSHandler::IsConnected,
                                           base::Unretained(this)));
  BindFunctionToObject(isolate, ethereum_object, "enable",
                       base::BindRepeating(&BraveWalletJSHandler::Enable,
                                           base::Unretained(this)));
  BindFunctionToObject(isolate, ethereum_object, "sendAsync",
                       base::BindRepeating(&BraveWalletJSHandler::SendAsync,
                                           base::Unretained(this)));
  BindFunctionToObject(
      isolate, ethereum_object, "send",
      base::BindRepeating(&BraveWalletJSHandler::Send, base::Unretained(this)));
  BindFunctionToObject(isolate, metamask_object, "isUnlocked",
                       base::BindRepeating(&BraveWalletJSHandler::IsUnlocked,
                                           base::Unretained(this)));
}

template <typename Sig>
void BraveWalletJSHandler::BindFunctionToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> javascript_object,
    const std::string& name,
    const base::RepeatingCallback<Sig>& callback) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  // Get the isolate associated with this object.
  javascript_object
      ->Set(context, gin::StringToSymbol(isolate, name),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

void BraveWalletJSHandler::ContinueEthSendTransaction(
    const std::string& normalized_json_request,
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    mojom::EthereumChainPtr chain,
    mojom::KeyringInfoPtr keyring_info) {
  if (!chain || !keyring_info) {
    mojom::ProviderError code = mojom::ProviderError::kInternalError;
    std::string message = "Internal JSON-RPC error";
    std::unique_ptr<base::Value> formed_response =
        GetProviderErrorDictionary(code, message);
    SendResponse(std::move(id), std::move(global_context),
                 std::move(global_callback), std::move(promise_resolver),
                 isolate, force_json_response, std::move(formed_response),
                 false);
    return;
  }

  std::string from;
  mojom::TxData1559Ptr tx_data_1559 =
      ParseEthSendTransaction1559Params(normalized_json_request, &from);
  if (!tx_data_1559) {
    mojom::ProviderError code = mojom::ProviderError::kInternalError;
    std::string message = "Internal JSON-RPC error";
    std::unique_ptr<base::Value> formed_response =
        GetProviderErrorDictionary(code, message);
    SendResponse(std::move(id), std::move(global_context),
                 std::move(global_callback), std::move(promise_resolver),
                 isolate, force_json_response, std::move(formed_response),
                 false);
    return;
  }

  if (ShouldCreate1559Tx(tx_data_1559.Clone(), chain->is_eip1559,
                         keyring_info->account_infos, from)) {
    // Set chain_id to current chain_id.
    tx_data_1559->chain_id = chain->chain_id;
    brave_wallet_provider_->AddAndApprove1559Transaction(
        std::move(tx_data_1559), from,
        base::BindOnce(&BraveWalletJSHandler::OnAddAndApproveTransaction,
                       weak_ptr_factory_.GetWeakPtr(), std::move(id),
                       std::move(global_context), std::move(global_callback),
                       std::move(promise_resolver), isolate,
                       force_json_response));
  } else {
    brave_wallet_provider_->AddAndApproveTransaction(
        std::move(tx_data_1559->base_data), from,
        base::BindOnce(&BraveWalletJSHandler::OnAddAndApproveTransaction,
                       weak_ptr_factory_.GetWeakPtr(), std::move(id),
                       std::move(global_context), std::move(global_callback),
                       std::move(promise_resolver), isolate,
                       force_json_response));
  }
}

bool BraveWalletJSHandler::CommonRequestOrSendAsync(
    std::unique_ptr<base::Value> input_value,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    mojom::ProviderError* error,
    std::string* error_message) {
  if (!EnsureConnected() || !input_value)
    return false;

  std::string input_json;
  if (!base::JSONWriter::Write(*input_value, &input_json))
    return false;

  std::string normalized_json_request;
  if (!NormalizeEthRequest(input_json, &normalized_json_request))
    return false;

  base::Value id;
  std::string method;
  if (!GetEthJsonRequestInfo(normalized_json_request, &id, &method, nullptr))
    return false;

  if (method == kEthAccounts) {
    brave_wallet_provider_->GetAllowedAccounts(
        false, base::BindOnce(
                   &BraveWalletJSHandler::OnGetAllowedAccounts,
                   weak_ptr_factory_.GetWeakPtr(), std::move(id),
                   std::move(global_context), std::move(global_callback),
                   std::move(promise_resolver), isolate, force_json_response));
  } else if (method == kEthRequestAccounts) {
    brave_wallet_provider_->RequestEthereumPermissions(base::BindOnce(
        &BraveWalletJSHandler::OnEthereumPermissionRequested,
        weak_ptr_factory_.GetWeakPtr(), std::move(id),
        std::move(global_context), std::move(global_callback),
        std::move(promise_resolver), isolate, force_json_response));
  } else if (method == kAddEthereumChainMethod) {
    brave_wallet_provider_->AddEthereumChain(
        normalized_json_request,
        base::BindOnce(&BraveWalletJSHandler::OnAddOrSwitchEthereumChain,
                       weak_ptr_factory_.GetWeakPtr(), std::move(id),
                       std::move(global_context), std::move(global_callback),
                       std::move(promise_resolver), isolate,
                       force_json_response));
  } else if (method == kSwitchEthereumChainMethod) {
    std::string chain_id;
    if (!ParseSwitchEthereumChainParams(normalized_json_request, &chain_id))
      return false;
    brave_wallet_provider_->SwitchEthereumChain(
        chain_id,
        base::BindOnce(&BraveWalletJSHandler::OnAddOrSwitchEthereumChain,
                       weak_ptr_factory_.GetWeakPtr(), std::move(id),
                       std::move(global_context), std::move(global_callback),
                       std::move(promise_resolver), isolate,
                       force_json_response));
  } else if (method == kEthSendTransaction) {
    brave_wallet_provider_->GetNetworkAndDefaultKeyringInfo(base::BindOnce(
        &BraveWalletJSHandler::ContinueEthSendTransaction,
        weak_ptr_factory_.GetWeakPtr(), normalized_json_request, std::move(id),
        std::move(global_context), std::move(global_callback),
        std::move(promise_resolver), isolate, force_json_response));
  } else if (method == kEthSign || method == kPersonalSign) {
    std::string address;
    std::string message;
    if (method == kPersonalSign &&
        !ParsePersonalSignParams(normalized_json_request, &address, &message)) {
      return false;
    } else if (method == kEthSign &&
               !ParseEthSignParams(normalized_json_request, &address,
                                   &message)) {
      return false;
    }

    brave_wallet_provider_->SignMessage(
        address, message,
        base::BindOnce(&BraveWalletJSHandler::OnSignRecoverMessage,
                       weak_ptr_factory_.GetWeakPtr(), std::move(id),
                       std::move(global_context), std::move(global_callback),
                       std::move(promise_resolver), isolate,
                       force_json_response));
  } else if (method == kPersonalEcRecover) {
    std::string message;
    std::string signature;
    if (!ParsePersonalEcRecoverParams(normalized_json_request, &message,
                                      &signature)) {
      return false;
    }
    brave_wallet_provider_->RecoverAddress(
        message, signature,
        base::BindOnce(&BraveWalletJSHandler::OnSignRecoverMessage,
                       weak_ptr_factory_.GetWeakPtr(), std::move(id),
                       std::move(global_context), std::move(global_callback),
                       std::move(promise_resolver), isolate,
                       force_json_response));
  } else if (method == kEthSignTypedDataV3 || method == kEthSignTypedDataV4) {
    std::string address;
    std::string message;
    base::Value domain;
    std::vector<uint8_t> message_to_sign;
    if (method == kEthSignTypedDataV4) {
      if (!ParseEthSignTypedDataParams(normalized_json_request, &address,
                                       &message, &message_to_sign, &domain,
                                       EthSignTypedDataHelper::Version::kV4))
        return false;
    } else {
      if (!ParseEthSignTypedDataParams(normalized_json_request, &address,
                                       &message, &message_to_sign, &domain,
                                       EthSignTypedDataHelper::Version::kV3))
        return false;
    }
    brave_wallet_provider_->SignTypedMessage(
        address, message, base::HexEncode(message_to_sign), std::move(domain),
        base::BindOnce(&BraveWalletJSHandler::OnSignRecoverMessage,
                       weak_ptr_factory_.GetWeakPtr(), std::move(id),
                       std::move(global_context), std::move(global_callback),
                       std::move(promise_resolver), isolate,
                       force_json_response));
  } else if (method == kWalletWatchAsset || method == kMetamaskWatchAsset) {
    mojom::BlockchainTokenPtr token;
    if (!ParseWalletWatchAssetParams(normalized_json_request, &token,
                                     error_message)) {
      if (error_message && !error_message->empty() && error)
        *error = mojom::ProviderError::kInvalidParams;
      return false;
    }
    brave_wallet_provider_->AddSuggestToken(
        std::move(token),
        base::BindOnce(&BraveWalletJSHandler::OnAddSuggestToken,
                       weak_ptr_factory_.GetWeakPtr(), std::move(id),
                       std::move(global_context), std::move(global_callback),
                       std::move(promise_resolver), isolate,
                       force_json_response));
  } else if (method == kRequestPermissionsMethod) {
    std::vector<std::string> restricted_methods;
    if (!ParseRequestPermissionsParams(normalized_json_request,
                                       &restricted_methods))
      return false;
    if (std::find(restricted_methods.begin(), restricted_methods.end(),
                  "eth_accounts") == restricted_methods.end())
      return false;
    brave_wallet_provider_->RequestEthereumPermissions(base::BindOnce(
        &BraveWalletJSHandler::OnRequestPermissionsAccountsRequested,
        weak_ptr_factory_.GetWeakPtr(), std::move(id),
        std::move(global_context), std::move(global_callback),
        std::move(promise_resolver), isolate, force_json_response));
  } else if (method == kGetPermissionsMethod) {
    brave_wallet_provider_->GetAllowedAccounts(
        true, base::BindOnce(
                  &BraveWalletJSHandler::OnGetGetPermissionsAccountsRequested,
                  weak_ptr_factory_.GetWeakPtr(), std::move(id),
                  std::move(global_context), std::move(global_callback),
                  std::move(promise_resolver), isolate, force_json_response));
  } else {
    brave_wallet_provider_->Request(
        normalized_json_request, true,
        base::BindOnce(&BraveWalletJSHandler::BraveWalletJSHandler::
                           OnCommonRequestOrSendAsync,
                       weak_ptr_factory_.GetWeakPtr(), std::move(id),
                       std::move(global_context), std::move(global_callback),
                       std::move(promise_resolver), isolate,
                       force_json_response));
  }
  return true;
}

// There are 3 supported signatures for send:
//
// 1) ethereum.send(payload: JsonRpcRequest, callback: JsonRpcCallback): void;
// Same as ethereum.sendAsync()
//
// 2) ethereum.send(method: string, params?: Array<unknown>):
// Promise<JsonRpcResponse>; method and parameters specified instead of inside
// a JSON-RPC payload
//
// 3) ethereum.send(payload: JsonRpcRequest): unknown;
// Only valid for: eth_accounts, eth_coinbase, eth_uninstallFilter, etc.
v8::Local<v8::Promise> BraveWalletJSHandler::Send(gin::Arguments* args) {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();
  v8::Isolate* isolate = args->isolate();
  if (args->Length() == 0) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  // Method signature 1, so just handle this as sendAsync()
  v8::Local<v8::Value> arg1 = args->PeekNext();
  if (arg1->IsObject()) {
    SendAsync(args);
    return v8::Local<v8::Promise>();
  }

  if (!args->GetNext(&arg1)) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  std::unique_ptr<base::Value> arg1_value =
      content::V8ValueConverter::Create()->FromV8Value(
          arg1, isolate->GetCurrentContext());
  if (!arg1_value) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  // At this point we must have signature 2 or signature 3
  // And in either case, arg1 must be a string.
  if (!arg1_value->is_string()) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  std::string method = arg1_value->GetString();
  bool supported_single_arg_function =
      method == "net_listening" || method == "net_peerCount" ||
      method == "net_version" || method == "eth_chainId" ||
      method == "eth_syncing" || method == "eth_coinbase" ||
      method == "eth_mining" || method == "eth_hashrate" ||
      method == "eth_accounts" || method == "eth_newBlockFilter" ||
      method == "eth_newPendingTransactionFilter";

  if (args->Length() == 1 && !supported_single_arg_function) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  std::unique_ptr<base::Value> params;
  if (args->Length() > 1) {
    v8::Local<v8::Value> arg2;
    if (!args->GetNext(&arg2)) {
      args->ThrowError();
      return v8::Local<v8::Promise>();
    }
    params = content::V8ValueConverter::Create()->FromV8Value(
        arg2, isolate->GetCurrentContext());
    if (!params || !params->is_list()) {
      args->ThrowError();
      return v8::Local<v8::Promise>();
    }
  } else {
    // supported_single_arg_function
    params = base::Value::ToUniquePtrValue(base::ListValue());
  }

  // There's no id in this format so we can just use 1
  return RequestBaseValue(isolate, GetJsonRpcRequest(method, std::move(params)),
                          true);
}

void BraveWalletJSHandler::SendAsync(gin::Arguments* args) {
  if (!EnsureConnected())
    return;
  v8::Isolate* isolate = args->isolate();
  v8::Local<v8::Value> input;
  v8::Local<v8::Function> callback;
  if (!args->GetNext(&input) || !args->GetNext(&callback)) {
    args->ThrowError();
    return;
  }

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto global_callback =
      std::make_unique<v8::Global<v8::Function>>(isolate, callback);
  std::unique_ptr<base::Value> input_value =
      content::V8ValueConverter::Create()->FromV8Value(
          input, isolate->GetCurrentContext());
  mojom::ProviderError code = mojom::ProviderError::kUnsupportedMethod;
  std::string message = "Generic processing error";
  if (!CommonRequestOrSendAsync(
          std::move(input_value), std::move(global_context),
          std::move(global_callback), v8::Global<v8::Promise::Resolver>(),
          isolate, true, &code, &message)) {
    base::Value id;
    std::string input_json;
    input_value = content::V8ValueConverter::Create()->FromV8Value(
        input, isolate->GetCurrentContext());
    if (base::JSONWriter::Write(*input_value, &input_json)) {
      std::string method;
      ALLOW_UNUSED_LOCAL(
          GetEthJsonRequestInfo(input_json, &id, nullptr, nullptr));
    }

    auto global_context(
        v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
    auto global_callback =
        std::make_unique<v8::Global<v8::Function>>(isolate, callback);
    auto formed_response = GetProviderErrorDictionary(code, message);
    SendResponse(std::move(id), std::move(global_context),
                 std::move(global_callback),
                 v8::Global<v8::Promise::Resolver>(), isolate, true,
                 std::move(formed_response), false);
  }
}

v8::Local<v8::Value> BraveWalletJSHandler::IsConnected() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame_->GetWebFrame()->MainWorldScriptContext();
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  return v8::Boolean::New(isolate, is_connected_);
}

v8::Local<v8::Promise> BraveWalletJSHandler::Request(
    v8::Isolate* isolate,
    v8::Local<v8::Value> input) {
  if (!input->IsObject())
    return v8::Local<v8::Promise>();
  std::unique_ptr<base::Value> input_value =
      content::V8ValueConverter::Create()->FromV8Value(
          input, isolate->GetCurrentContext());
  return RequestBaseValue(isolate, std::move(input_value), false);
}

v8::Local<v8::Promise> BraveWalletJSHandler::RequestBaseValue(
    v8::Isolate* isolate,
    std::unique_ptr<base::Value> input_value,
    bool force_json_response) {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();

  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }
  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));

  mojom::ProviderError code = mojom::ProviderError::kUnsupportedMethod;
  std::string message = "Generic processing error";
  if (!CommonRequestOrSendAsync(std::move(input_value),
                                std::move(global_context), nullptr,
                                std::move(promise_resolver), isolate,
                                force_json_response, &code, &message)) {
    auto formed_response = GetProviderErrorDictionary(code, message);
    auto global_context(
        v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
    auto promise_resolver(
        v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
    SendResponse(base::Value(), std::move(global_context), nullptr,
                 std::move(promise_resolver), isolate, force_json_response,
                 std::move(formed_response), false);
  }

  return resolver.ToLocalChecked()->GetPromise();
}

void BraveWalletJSHandler::OnCommonRequestOrSendAsync(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    const int http_code,
    const std::string& response,
    const base::flat_map<std::string, std::string>& headers) {
  bool reject;
  std::unique_ptr<base::Value> formed_response =
      GetProviderRequestReturnFromEthJsonResponse(http_code, response, &reject);
  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response), !reject);
}

v8::Local<v8::Promise> BraveWalletJSHandler::Enable() {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  auto context(v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  brave_wallet_provider_->RequestEthereumPermissions(base::BindOnce(
      &BraveWalletJSHandler::OnEthereumPermissionRequested,
      weak_ptr_factory_.GetWeakPtr(), base::Value(), std::move(global_context),
      nullptr, std::move(promise_resolver), isolate, false));

  return resolver.ToLocalChecked()->GetPromise();
}

v8::Local<v8::Promise> BraveWalletJSHandler::IsUnlocked() {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  auto context(v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  brave_wallet_provider_->IsLocked(base::BindOnce(
      &BraveWalletJSHandler::OnIsUnlocked, weak_ptr_factory_.GetWeakPtr(),
      std::move(global_context), std::move(promise_resolver), isolate));

  return resolver.ToLocalChecked()->GetPromise();
}

void BraveWalletJSHandler::ExecuteScript(const std::string script) {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  if (web_frame->IsProvisional())
    return;

  web_frame->ExecuteScript(
      blink::WebScriptSource(blink::WebString::FromUTF8(script)));
}

void BraveWalletJSHandler::InjectInitScript() {
  ExecuteScript(*g_provider_script);
  if (!allow_overwrite_window_ethereum_) {
    ExecuteScript(kEthereumNonWritable);
  }
}

void BraveWalletJSHandler::FireEvent(const std::string& event,
                                     base::Value event_args) {
  base::Value args = base::Value(base::Value::Type::LIST);
  args.Append(event);
  args.Append(std::move(event_args));
  CallMethodOfObject(render_frame_->GetWebFrame(), u"ethereum", u"emit",
                     std::move(args));
}

void BraveWalletJSHandler::ConnectEvent() {
  if (!EnsureConnected())
    return;

  brave_wallet_provider_->GetChainId(base::BindOnce(
      &BraveWalletJSHandler::OnGetChainId, weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletJSHandler::OnGetChainId(const std::string& chain_id) {
  base::DictionaryValue event_args;
  event_args.SetStringKey("chainId", chain_id);
  FireEvent(kConnectEvent, std::move(event_args));
  is_connected_ = true;
  chain_id_ = chain_id;
  UpdateAndBindJSProperties();
}

void BraveWalletJSHandler::DisconnectEvent(const std::string& message) {
  // FireEvent(kDisconnectEvent, message);
}

void BraveWalletJSHandler::ChainChangedEvent(const std::string& chain_id) {
  if (chain_id_ == chain_id)
    return;

  FireEvent(kChainChangedEvent, base::Value(chain_id));
  chain_id_ = chain_id;
  UpdateAndBindJSProperties();
}

void BraveWalletJSHandler::AccountsChangedEvent(
    const std::vector<std::string>& accounts) {
  base::ListValue event_args;
  for (const std::string& account : accounts) {
    event_args.Append(base::Value(account));
  }
  first_allowed_account_.clear();
  if (accounts.size() > 0) {
    first_allowed_account_ = accounts[0];
  }
  UpdateAndBindJSProperties();
  FireEvent(kAccountsChangedEvent, std::move(event_args));
}

}  // namespace brave_wallet
