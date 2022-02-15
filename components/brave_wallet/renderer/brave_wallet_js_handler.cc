/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_js_handler.h"

#include <limits>
#include <tuple>
#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
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
#include "url/origin.h"

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
  for (auto const& argument : arguments.GetListDeprecated()) {
    args.push_back(
        content::V8ValueConverter::Create()->ToV8Value(&argument, context));
  }

  web_frame->ExecuteMethodAndReturnValue(v8::Local<v8::Function>::Cast(method),
                                         object, static_cast<int>(args.size()),
                                         args.data());
}

}  // namespace

namespace brave_wallet {

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
  std::ignore = resolver->Resolve(context, local_result);
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
    std::ignore = resolver->Resolve(context, result);
  } else {
    std::ignore = resolver->Reject(context, result);
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

void BraveWalletJSHandler::AllowOverwriteWindowEthereum(bool allow) {
  allow_overwrite_window_ethereum_ = allow;
}

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

  // There's no id in this format so we can just use 1
  brave_wallet_provider_->Send(
      method, std::move(*params),
      url::Origin(render_frame_->GetWebFrame()->GetSecurityOrigin())
          .Serialize(),
      base::BindOnce(&BraveWalletJSHandler::OnRequestOrSendAsync,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     nullptr, std::move(promise_resolver), isolate, true));

  return resolver.ToLocalChecked()->GetPromise();
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

  brave_wallet_provider_->Request(
      std::move(*input_value),
      url::Origin(render_frame_->GetWebFrame()->GetSecurityOrigin())
          .Serialize(),
      base::BindOnce(
          &BraveWalletJSHandler::BraveWalletJSHandler::OnRequestOrSendAsync,
          weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
          std::move(global_callback), v8::Global<v8::Promise::Resolver>(),
          isolate, true));
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

  brave_wallet_provider_->Request(
      std::move(*input_value),
      url::Origin(render_frame_->GetWebFrame()->GetSecurityOrigin())
          .Serialize(),
      base::BindOnce(
          &BraveWalletJSHandler::BraveWalletJSHandler::OnRequestOrSendAsync,
          weak_ptr_factory_.GetWeakPtr(), std::move(global_context), nullptr,
          std::move(promise_resolver), isolate, false));

  return resolver.ToLocalChecked()->GetPromise();
}

void BraveWalletJSHandler::OnRequestOrSendAsync(
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    base::Value id,
    base::Value formed_response,
    const bool reject,
    const std::string& first_allowed_account,
    const bool update_bind_js_properties) {
  if (update_bind_js_properties) {
    first_allowed_account_ = first_allowed_account;
    UpdateAndBindJSProperties();
  }
  SendResponse(
      std::move(id), std::move(global_context), std::move(global_callback),
      std::move(promise_resolver), isolate, force_json_response,
      base::Value::ToUniquePtrValue(std::move(formed_response)), !reject);
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

  brave_wallet_provider_->Enable(
      base::BindOnce(&BraveWalletJSHandler::OnRequestOrSendAsync,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
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
