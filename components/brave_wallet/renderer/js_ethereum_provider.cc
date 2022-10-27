/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_ethereum_provider.h"

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
#include "brave/components/brave_wallet/renderer/resource_helper.h"
#include "brave/components/brave_wallet/renderer/v8_helper.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_script_generated.h"
#include "content/public/common/isolated_world_ids.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/origin.h"

namespace {

static base::NoDestructor<std::string> g_provider_script("");

constexpr char kEthereum[] = "ethereum";
constexpr char kEmit[] = "emit";
constexpr char kIsBraveWallet[] = "isBraveWallet";
constexpr char kEthereumProviderScript[] = "ethereum_provider.js";
constexpr char kIsMetaMask[] = "isMetaMask";
constexpr char kMetaMask[] = "_metamask";
constexpr char kIsUnlocked[] = "isUnlocked";

}  // namespace

namespace brave_wallet {

void JSEthereumProvider::OnIsUnlocked(
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
      content::V8ValueConverter::Create()->ToV8Value(result, context);
  std::ignore = resolver->Resolve(context, local_result);
}

void JSEthereumProvider::SendResponse(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    base::Value formed_response,
    bool success) {
  if (!render_frame())
    return;
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);

  if (global_callback || force_json_response) {
    auto full_formed_response = brave_wallet::ToProviderResponse(
        std::move(id), success ? &formed_response : nullptr,
        success ? nullptr : &formed_response);
    formed_response = std::move(full_formed_response);
  }

  v8::Local<v8::Value> result =
      content::V8ValueConverter::Create()->ToV8Value(formed_response, context);
  if (global_callback) {
    v8::Local<v8::Value> result_null = v8::Null(isolate);
    v8::Local<v8::Value> argv[] = {success ? result_null : result,
                                   success ? result : result_null};
    v8::Local<v8::Function> callback_local =
        v8::Local<v8::Function>::New(isolate, *global_callback);
    render_frame()->GetWebFrame()->CallFunctionEvenIfScriptDisabled(
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

JSEthereumProvider::JSEthereumProvider(content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {
  if (g_provider_script->empty()) {
    *g_provider_script = LoadDataResource(
        IDR_BRAVE_WALLET_SCRIPT_ETHEREUM_PROVIDER_SCRIPT_BUNDLE_JS);
  }
  EnsureConnected();
}

JSEthereumProvider::~JSEthereumProvider() = default;

gin::WrapperInfo JSEthereumProvider::kWrapperInfo = {gin::kEmbedderNativeGin};

void JSEthereumProvider::WillReleaseScriptContext(v8::Local<v8::Context>,
                                                  int32_t world_id) {
  if (world_id != content::ISOLATED_WORLD_ID_GLOBAL)
    return;
  // Close mojo connection from browser to renderer.
  receiver_.reset();
}

bool JSEthereumProvider::EnsureConnected() {
  if (!render_frame())
    return false;

  if (!ethereum_provider_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker()->GetInterface(
        ethereum_provider_.BindNewPipeAndPassReceiver());
    ethereum_provider_->Init(receiver_.BindNewPipeAndPassRemote());
  }

  return ethereum_provider_.is_bound();
}

// static
void JSEthereumProvider::Install(bool allow_overwrite_window_ethereum_provider,
                                 content::RenderFrame* render_frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame->GetWebFrame()->MainWorldScriptContext();
  if (context.IsEmpty())
    return;
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Value> ethereum_value;
  if (!global->Get(context, gin::StringToV8(isolate, kEthereum))
           .ToLocal(&ethereum_value) ||
      !ethereum_value->IsObject()) {
    gin::Handle<JSEthereumProvider> provider =
        gin::CreateHandle(isolate, new JSEthereumProvider(render_frame));
    if (provider.IsEmpty())
      return;
    v8::Local<v8::Value> provider_value = provider.ToV8();

    if (!allow_overwrite_window_ethereum_provider) {
      SetProviderNonWritable(context, global, provider_value,
                             gin::StringToV8(isolate, kEthereum), true);
    } else {
      global
          ->Set(context, gin::StringToSymbol(isolate, kEthereum),
                provider_value)
          .Check();
    }

    v8::Local<v8::Object> provider_object =
        provider_value->ToObject(context).ToLocalChecked();

    // Non-function properties are readonly guaranteed by gin::Wrappable
    // send should be writable because of
    // https://github.com/brave/brave-browser/issues/25078
    for (const std::string& method :
         {"request", "isConnected", "enable", "sendAsync"}) {
      SetOwnPropertyWritable(context, provider_object,
                             gin::StringToV8(isolate, method), false);
    }

    // Set isMetaMask to writable.
    // isMetaMask should be writable because of
    // https://github.com/brave/brave-browser/issues/22213;
    SetOwnPropertyWritable(context, provider_object,
                           gin::StringToV8(isolate, kIsMetaMask), true);

    // Set non-writable _metamask obj with non-writable isUnlocked method.
    v8::Local<v8::Value> metamask_value;
    v8::Local<v8::Object> metamask_obj = v8::Object::New(isolate);
    provider_object
        ->Set(context, gin::StringToSymbol(isolate, kMetaMask), metamask_obj)
        .Check();
    SetOwnPropertyWritable(context, provider_object,
                           gin::StringToV8(isolate, kMetaMask), false);

    metamask_obj
        ->Set(
            context, gin::StringToSymbol(isolate, kIsUnlocked),
            gin::CreateFunctionTemplate(
                isolate, base::BindRepeating(&JSEthereumProvider::IsUnlocked,
                                             base::Unretained(provider.get())))
                ->GetFunction(context)
                .ToLocalChecked())
        .Check();
    SetOwnPropertyWritable(context, metamask_obj,
                           gin::StringToV8(isolate, kIsUnlocked), false);

    blink::WebLocalFrame* web_frame = render_frame->GetWebFrame();
    ExecuteScript(web_frame, *g_provider_script, kEthereumProviderScript);
    provider->ConnectEvent();
  } else {
    render_frame->GetWebFrame()->AddMessageToConsole(
        blink::WebConsoleMessage(blink::mojom::ConsoleMessageLevel::kWarning,
                                 "Brave Wallet will not insert window.ethereum "
                                 "because it already exists!"));
  }
}

bool JSEthereumProvider::GetIsBraveWallet() {
  return true;
}

bool JSEthereumProvider::GetIsMetaMask() {
  return true;
}

std::string JSEthereumProvider::GetChainId() {
  return chain_id_;
}

v8::Local<v8::Value> JSEthereumProvider::GetNetworkVersion(
    v8::Isolate* isolate) {
  // We have no easy way to convert a uin256 to a decimal number string yet
  // and this is a deprecated property so it's not very important.
  // So we only include it when the chain ID is <= uint64_t max value.
  uint256_t chain_id_uint256;
  if (HexValueToUint256(chain_id_, &chain_id_uint256) &&
      chain_id_uint256 <= (uint256_t)std::numeric_limits<uint64_t>::max()) {
    uint64_t networkVersion = (uint64_t)chain_id_uint256;
    return gin::StringToV8(isolate, std::to_string(networkVersion));
  }

  return v8::Undefined(isolate);
}

v8::Local<v8::Value> JSEthereumProvider::GetSelectedAddress(
    v8::Isolate* isolate) {
  // Note this does not return the selected account, but it returns the first
  // connected account that was given permissions.
  if (first_allowed_account_.empty())
    return v8::Undefined(isolate);
  return gin::StringToV8(isolate, first_allowed_account_);
}

gin::ObjectTemplateBuilder JSEthereumProvider::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<JSEthereumProvider>::GetObjectTemplateBuilder(isolate)
      .SetProperty(kIsBraveWallet, &JSEthereumProvider::GetIsBraveWallet)
      .SetProperty(kIsMetaMask, &JSEthereumProvider::GetIsMetaMask)
      .SetProperty("chainId", &JSEthereumProvider::GetChainId)
      .SetProperty("networkVersion", &JSEthereumProvider::GetNetworkVersion)
      .SetProperty("selectedAddress", &JSEthereumProvider::GetSelectedAddress)
      .SetMethod("request", &JSEthereumProvider::Request)
      .SetMethod("isConnected", &JSEthereumProvider::IsConnected)
      .SetMethod("enable", &JSEthereumProvider::Enable)
      .SetMethod("sendAsync", &JSEthereumProvider::SendAsync)
      .SetMethod("send", &JSEthereumProvider::SendMethod);
}

const char* JSEthereumProvider::GetTypeName() {
  return "JSEthereumProvider";
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
v8::Local<v8::Promise> JSEthereumProvider::SendMethod(gin::Arguments* args) {
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
  ethereum_provider_->Send(
      method, std::move(*params),
      base::BindOnce(&JSEthereumProvider::OnRequestOrSendAsync,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     nullptr, std::move(promise_resolver), isolate, true));

  return resolver.ToLocalChecked()->GetPromise();
}

void JSEthereumProvider::SendAsync(gin::Arguments* args) {
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

  ethereum_provider_->Request(
      std::move(*input_value),
      base::BindOnce(
          &JSEthereumProvider::JSEthereumProvider::OnRequestOrSendAsync,
          weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
          std::move(global_callback), v8::Global<v8::Promise::Resolver>(),
          isolate, true));
}

bool JSEthereumProvider::IsConnected() {
  return is_connected_;
}

v8::Local<v8::Promise> JSEthereumProvider::Request(v8::Isolate* isolate,
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

  ethereum_provider_->Request(
      std::move(*input_value),
      base::BindOnce(
          &JSEthereumProvider::JSEthereumProvider::OnRequestOrSendAsync,
          weak_ptr_factory_.GetWeakPtr(), std::move(global_context), nullptr,
          std::move(promise_resolver), isolate, false));

  return resolver.ToLocalChecked()->GetPromise();
}

void JSEthereumProvider::OnRequestOrSendAsync(
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
  }
  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response), !reject);
}

v8::Local<v8::Promise> JSEthereumProvider::Enable(v8::Isolate* isolate) {
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
  auto context(v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));

  ethereum_provider_->Enable(
      base::BindOnce(&JSEthereumProvider::OnRequestOrSendAsync,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     nullptr, std::move(promise_resolver), isolate, false));

  return resolver.ToLocalChecked()->GetPromise();
}

v8::Local<v8::Promise> JSEthereumProvider::IsUnlocked(v8::Isolate* isolate) {
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
  auto context(v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  ethereum_provider_->IsLocked(base::BindOnce(
      &JSEthereumProvider::OnIsUnlocked, weak_ptr_factory_.GetWeakPtr(),
      std::move(global_context), std::move(promise_resolver), isolate));

  return resolver.ToLocalChecked()->GetPromise();
}

void JSEthereumProvider::FireEvent(const std::string& event,
                                   base::Value event_args) {
  if (!render_frame())
    return;
  base::Value::List args_list;
  args_list.Append(event);
  args_list.Append(std::move(event_args));

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame()->GetWebFrame()->MainWorldScriptContext();

  std::vector<v8::Local<v8::Value>> args;
  for (auto const& argument : args_list) {
    args.push_back(
        content::V8ValueConverter::Create()->ToV8Value(argument, context));
  }
  CallMethodOfObject(render_frame()->GetWebFrame(), kEthereum, kEmit,
                     std::move(args));
}

void JSEthereumProvider::ConnectEvent() {
  if (!EnsureConnected())
    return;

  ethereum_provider_->GetChainId(base::BindOnce(
      &JSEthereumProvider::OnGetChainId, weak_ptr_factory_.GetWeakPtr()));
}

void JSEthereumProvider::OnGetChainId(const std::string& chain_id) {
  base::Value::Dict event_args;
  event_args.Set("chainId", chain_id);
  FireEvent(kConnectEvent, base::Value(std::move(event_args)));
  is_connected_ = true;
  chain_id_ = chain_id;
}

void JSEthereumProvider::DisconnectEvent(const std::string& message) {
  // FireEvent(kDisconnectEvent, message);
}

void JSEthereumProvider::ChainChangedEvent(const std::string& chain_id) {
  if (chain_id_ == chain_id)
    return;

  FireEvent(ethereum::kChainChangedEvent, base::Value(chain_id));
  chain_id_ = chain_id;
}

void JSEthereumProvider::AccountsChangedEvent(
    const std::vector<std::string>& accounts) {
  base::ListValue event_args;
  for (const std::string& account : accounts) {
    event_args.Append(base::Value(account));
  }
  first_allowed_account_.clear();
  if (accounts.size() > 0) {
    first_allowed_account_ = accounts[0];
  }
  FireEvent(ethereum::kAccountsChangedEvent, std::move(event_args));
}

}  // namespace brave_wallet
