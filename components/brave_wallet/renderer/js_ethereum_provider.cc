/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_ethereum_provider.h"

#include <limits>
#include <tuple>
#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "base/uuid.h"
#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "brave/components/brave_wallet/renderer/resource_helper.h"
#include "brave/components/brave_wallet/renderer/v8_helper.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_script_generated.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/common/isolated_world_ids.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/abseil-cpp/absl/base/macros.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

namespace {

constexpr char kBraveEthereum[] = "braveEthereum";
constexpr char kEthereum[] = "ethereum";
constexpr char kEmit[] = "emit";
constexpr char kIsBraveWallet[] = "isBraveWallet";
constexpr char kEthereumProxyHandlerScript[] = R"((function() {
  const handler = {
    get: (target, property, receiver) => {
      const value = target[property];
      if (typeof value === 'function' &&
          (property === 'request' || property === 'isConnected' ||
           property === 'enable' || property === 'sendAsync' ||
           property === 'send')) {
        return new Proxy(value, {
          apply: (targetFunc, thisArg, args) => {
            return targetFunc.call(target, ...args);
          }
        });
      }
      return value;
    }
  };
  return handler;
})())";
constexpr char kIsMetaMask[] = "isMetaMask";
constexpr char kMetaMask[] = "_metamask";
constexpr char kIsUnlocked[] = "isUnlocked";

}  // namespace

namespace brave_wallet {

void JSEthereumProvider::SendResponse(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    base::Value formed_response,
    bool success) {
  if (!render_frame()) {
    return;
  }
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

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
  uuid_ = base::Uuid::GenerateRandomV4().AsLowercaseString();
  EnsureConnected();
}

JSEthereumProvider::~JSEthereumProvider() = default;

gin::WrapperInfo JSEthereumProvider::kWrapperInfo = {gin::kEmbedderNativeGin};

void JSEthereumProvider::WillReleaseScriptContext(v8::Local<v8::Context>,
                                                  int32_t world_id) {
  if (world_id != content::ISOLATED_WORLD_ID_GLOBAL) {
    return;
  }
  // Close mojo connection from browser to renderer.
  receiver_.reset();
  script_context_released_ = true;
}

void JSEthereumProvider::DidDispatchDOMContentLoadedEvent() {
  if (script_context_released_) {
    return;
  }
  ConnectEvent();
}

bool JSEthereumProvider::EnsureConnected() {
  if (!render_frame()) {
    return false;
  }

  if (!ethereum_provider_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker().GetInterface(
        ethereum_provider_.BindNewPipeAndPassReceiver());
    ethereum_provider_->Init(receiver_.BindNewPipeAndPassRemote());
  }

  return ethereum_provider_.is_bound();
}

// static
void JSEthereumProvider::Install(bool install_ethereum_provider,
                                 bool allow_overwrite_window_ethereum_provider,
                                 content::RenderFrame* render_frame) {
  CHECK(render_frame);
  v8::Isolate* isolate =
      render_frame->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame->GetWebFrame()->MainWorldScriptContext();
  if (context.IsEmpty()) {
    return;
  }

  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Context::Scope context_scope(context);

  // Check window.braveEthereum existence.
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Value> brave_ethereum_value =
      global->Get(context, gin::StringToV8(isolate, kBraveEthereum))
          .ToLocalChecked();
  if (!brave_ethereum_value->IsUndefined()) {
    return;
  }

  gin::Handle<JSEthereumProvider> provider =
      gin::CreateHandle(isolate, new JSEthereumProvider(render_frame));
  if (provider.IsEmpty()) {
    return;
  }
  v8::Local<v8::Value> provider_value = provider.ToV8();
  v8::Local<v8::Object> provider_object =
      provider_value->ToObject(context).ToLocalChecked();

  // Create a proxy to the actual JSEthereumProvider object which will be
  // exposed via window.ethereum.
  // This proxy uses a handler which would call things directly on the actual
  // JSEthereumProvider object so dApps which creates and uses their own proxy
  // of window.ethereum to access our provider won't throw a "Illegal
  // invocation: Function must be called on an object of type
  // JSEthereumProvider" error.
  blink::WebLocalFrame* web_frame = render_frame->GetWebFrame();
  v8::Local<v8::Value> ethereum_proxy_handler_val;
  if (!ExecuteScript(web_frame, kEthereumProxyHandlerScript)
           .ToLocal(&ethereum_proxy_handler_val)) {
    return;
  }
  v8::Local<v8::Object> ethereum_proxy_handler_obj =
      ethereum_proxy_handler_val->ToObject(context).ToLocalChecked();
  v8::Local<v8::Proxy> ethereum_proxy;
  if (!v8::Proxy::New(context, provider_object, ethereum_proxy_handler_obj)
           .ToLocal(&ethereum_proxy)) {
    return;
  }

  // Set window.braveEthereum
  SetProviderNonWritable(context, global, ethereum_proxy,
                         gin::StringToV8(isolate, kBraveEthereum), true);

  // Set window.ethereumProvider
  {
    v8::Local<v8::Value> ethereum_value =
        global->Get(context, gin::StringToV8(isolate, kEthereum))
            .ToLocalChecked();
    if (install_ethereum_provider && ethereum_value->IsUndefined()) {
      if (!allow_overwrite_window_ethereum_provider) {
        SetProviderNonWritable(context, global, ethereum_proxy,
                               gin::StringToV8(isolate, kEthereum), true);
      } else {
        global
            ->Set(context, gin::StringToSymbol(isolate, kEthereum),
                  ethereum_proxy)
            .Check();
      }
    }
  }

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

  ExecuteScript(
      web_frame,
      LoadDataResource(
          IDR_BRAVE_WALLET_SCRIPT_ETHEREUM_PROVIDER_SCRIPT_BUNDLE_JS));

  provider->BindRequestProviderListener();
  provider->AnnounceProvider();
}

bool JSEthereumProvider::GetIsBraveWallet() {
  return true;
}

bool JSEthereumProvider::GetIsMetaMask() {
  return true;
}

gin::WrapperInfo JSEthereumProvider::MetaMask::kWrapperInfo = {
    gin::kEmbedderNativeGin};

JSEthereumProvider::MetaMask::MetaMask(content::RenderFrame* render_frame)
    : render_frame_(render_frame) {}
JSEthereumProvider::MetaMask::~MetaMask() = default;

gin::ObjectTemplateBuilder
JSEthereumProvider::MetaMask::GetObjectTemplateBuilder(v8::Isolate* isolate) {
  return gin::Wrappable<MetaMask>::GetObjectTemplateBuilder(isolate).SetMethod(
      kIsUnlocked, &JSEthereumProvider::MetaMask::IsUnlocked);
}

const char* JSEthereumProvider::MetaMask::GetTypeName() {
  return kMetaMask;
}

v8::Local<v8::Promise> JSEthereumProvider::MetaMask::IsUnlocked(
    v8::Isolate* isolate) {
  if (!ethereum_provider_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker().GetInterface(
        ethereum_provider_.BindNewPipeAndPassReceiver());
  }

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
      &JSEthereumProvider::MetaMask::OnIsUnlocked, base::Unretained(this),
      std::move(global_context), std::move(promise_resolver), isolate));

  return resolver.ToLocalChecked()->GetPromise();
}

void JSEthereumProvider::MetaMask::OnIsUnlocked(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool locked) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  base::Value result = base::Value(!locked);
  v8::Local<v8::Value> local_result =
      content::V8ValueConverter::Create()->ToV8Value(result, context);
  std::ignore = resolver->Resolve(context, local_result);
}

v8::Local<v8::Value> JSEthereumProvider::GetMetaMask(v8::Isolate* isolate) {
  // Set non-writable _metamask obj with non-writable isUnlocked method.
  gin::Handle<MetaMask> metamask =
      gin::CreateHandle(isolate, new MetaMask(render_frame()));
  if (metamask.IsEmpty()) {
    return v8::Undefined(isolate);
  }
  v8::Local<v8::Value> metamask_value = metamask.ToV8();
  SetOwnPropertyWritable(isolate->GetCurrentContext(),
                         metamask_value.As<v8::Object>(),
                         gin::StringToV8(isolate, kIsUnlocked), false);
  return metamask_value;
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
  if (first_allowed_account_.empty()) {
    return v8::Undefined(isolate);
  }
  return gin::StringToV8(isolate, first_allowed_account_);
}

gin::ObjectTemplateBuilder JSEthereumProvider::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  // Note: When adding a new method, you would need to update the list in
  // kEthereumProxyHandlerScript too otherwise the function call would fail.
  return gin::Wrappable<JSEthereumProvider>::GetObjectTemplateBuilder(isolate)
      .SetProperty(kIsBraveWallet, &JSEthereumProvider::GetIsBraveWallet)
      .SetProperty(kIsMetaMask, &JSEthereumProvider::GetIsMetaMask)
      .SetProperty(kMetaMask, &JSEthereumProvider::GetMetaMask)
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
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
  }
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
    params = std::make_unique<base::Value>(base::Value::Type::LIST);
  }

  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
  }

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
                     nullptr, std::move(promise_resolver), isolate));

  return resolver.ToLocalChecked()->GetPromise();
}

void JSEthereumProvider::SendAsync(gin::Arguments* args) {
  if (!EnsureConnected()) {
    return;
  }
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

  ethereum_provider_->SendAsync(
      std::move(*input_value),
      base::BindOnce(&JSEthereumProvider::OnRequestOrSendAsync,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(global_callback),
                     v8::Global<v8::Promise::Resolver>(), isolate));
}

bool JSEthereumProvider::IsConnected() {
  return is_connected_;
}

v8::Local<v8::Promise> JSEthereumProvider::Request(v8::Isolate* isolate,
                                                   v8::Local<v8::Value> input) {
  if (!input->IsObject()) {
    return v8::Local<v8::Promise>();
  }
  std::unique_ptr<base::Value> input_value =
      content::V8ValueConverter::Create()->FromV8Value(
          input, isolate->GetCurrentContext());

  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
  }

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
      base::BindOnce(&JSEthereumProvider::OnRequestOrSendAsync,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     nullptr, std::move(promise_resolver), isolate));

  return resolver.ToLocalChecked()->GetPromise();
}

void JSEthereumProvider::OnRequestOrSendAsync(
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
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
               std::move(formed_response), !reject);
}

v8::Local<v8::Promise> JSEthereumProvider::Enable(v8::Isolate* isolate) {
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
  }

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
                     nullptr, std::move(promise_resolver), isolate));

  return resolver.ToLocalChecked()->GetPromise();
}

void JSEthereumProvider::FireEvent(const std::string& event,
                                   base::ValueView event_args) {
  if (!render_frame()) {
    return;
  }
  base::Value event_name(event);
  base::ValueView args_list[] = {event_name, event_args};

  v8::Isolate* isolate =
      render_frame()->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame()->GetWebFrame()->MainWorldScriptContext();

  std::vector<v8::Local<v8::Value>> args;
  for (auto const& argument : args_list) {
    args.push_back(
        content::V8ValueConverter::Create()->ToV8Value(argument, context));
  }
  CallMethodOfObject(render_frame()->GetWebFrame(), kBraveEthereum, kEmit,
                     std::move(args));
}

void JSEthereumProvider::ConnectEvent() {
  if (!EnsureConnected()) {
    return;
  }

  ethereum_provider_->GetChainId(base::BindOnce(
      &JSEthereumProvider::OnGetChainId, weak_ptr_factory_.GetWeakPtr()));
}

void JSEthereumProvider::OnGetChainId(const std::string& chain_id) {
  base::Value::Dict event_args;
  event_args.Set("chainId", chain_id);
  FireEvent(kConnectEvent, event_args);
  is_connected_ = true;
  chain_id_ = chain_id;
}

void JSEthereumProvider::DisconnectEvent(const std::string& message) {
  // FireEvent(kDisconnectEvent, message);
}

void JSEthereumProvider::ChainChangedEvent(const std::string& chain_id) {
  if (chain_id_ == chain_id) {
    return;
  }

  FireEvent(ethereum::kChainChangedEvent, base::Value(chain_id));
  chain_id_ = chain_id;
}

void JSEthereumProvider::AccountsChangedEvent(
    const std::vector<std::string>& accounts) {
  base::Value::List event_args;
  for (const std::string& account : accounts) {
    event_args.Append(base::Value(account));
  }
  first_allowed_account_.clear();
  if (accounts.size() > 0) {
    first_allowed_account_ = accounts[0];
  }
  FireEvent(ethereum::kAccountsChangedEvent, event_args);
}

void JSEthereumProvider::MessageEvent(const std::string& subscription_id,
                                      base::Value result) {
  base::Value::Dict event_args;
  base::Value::Dict data;
  data.Set("subscription", subscription_id);
  data.Set("result", std::move(result));
  event_args.Set("type", "eth_subscription");
  event_args.Set("data", std::move(data));
  FireEvent(ethereum::kMessageEvent, event_args);
}

void JSEthereumProvider::OnProviderRequested() {
  AnnounceProvider();
}

void JSEthereumProvider::BindRequestProviderListener() {
  CHECK(render_frame());
  v8::Isolate* isolate =
      render_frame()->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame()->GetWebFrame()->MainWorldScriptContext();

  auto onProviderRequested = gin::CreateFunctionTemplate(
      isolate, base::BindRepeating(&JSEthereumProvider::OnProviderRequested,
                                   weak_ptr_factory_.GetWeakPtr()));
  auto functionInstance =
      onProviderRequested->GetFunction(context).ToLocalChecked();

  std::vector<v8::Local<v8::Value>> args;
  args.push_back(content::V8ValueConverter::Create()->ToV8Value(
      base::Value("eip6963:requestProvider"), context));
  args.push_back(std::move(functionInstance));

  CallMethodOfObject(render_frame()->GetWebFrame(), "window",
                     "addEventListener", std::move(args));
}

void JSEthereumProvider::AnnounceProvider() {
  CHECK(render_frame());
  v8::Isolate* isolate =
      render_frame()->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame()->GetWebFrame()->MainWorldScriptContext();

  base::Value::Dict provider_info_value;
  provider_info_value.Set("rdns", "com.brave.wallet");
  provider_info_value.Set("uuid", uuid_);
  provider_info_value.Set(
      "name", l10n_util::GetStringUTF8(IDS_WALLET_EIP6963_PROVIDER_NAME));
  provider_info_value.Set("icon", GetBraveWalletImage());

  auto detail = v8::Object::New(isolate);
  auto info_object = content::V8ValueConverter::Create()->ToV8Value(
      std::move(provider_info_value), context);

  if (!info_object.As<v8::Object>()
           ->SetIntegrityLevel(isolate->GetCurrentContext(),
                               v8::IntegrityLevel::kFrozen)
           .ToChecked()) {
    NOTREACHED_IN_MIGRATION();
    return;
  }

  if (detail
          ->Set(context,
                content::V8ValueConverter::Create()->ToV8Value(
                    base::Value("info"), context),
                std::move(info_object))
          .IsNothing()) {
    NOTREACHED_IN_MIGRATION();
    return;
  }

  auto provider =
      GetProperty(context, context->Global(), kBraveEthereum).ToLocalChecked();

  if (detail
          ->Set(context,
                content::V8ValueConverter::Create()->ToV8Value(
                    base::Value("provider"), context),
                provider)
          .IsNothing()) {
    NOTREACHED_IN_MIGRATION();
    return;
  }

  if (detail
          ->SetIntegrityLevel(isolate->GetCurrentContext(),
                              v8::IntegrityLevel::kFrozen)
          .IsNothing()) {
    NOTREACHED_IN_MIGRATION();
    return;
  }

  auto event_content = v8::Object::New(isolate);
  if (event_content
          ->Set(context,
                content::V8ValueConverter::Create()->ToV8Value(
                    base::Value("detail"), context),
                std::move(detail))
          .IsNothing()) {
    NOTREACHED_IN_MIGRATION();
    return;
  }

  v8::Local<v8::Function> custom_event =
      GetProperty(context, context->Global(), "CustomEvent")
          .ToLocalChecked()
          .As<v8::Function>();

  v8::Local<v8::Value> custom_event_args[] = {
      content::V8ValueConverter::Create()->ToV8Value(
          base::Value("eip6963:announceProvider"), context),
      std::move(event_content)};

  v8::Local<v8::Value> custom_event_value =
      custom_event
          ->NewInstance(context, ABSL_ARRAYSIZE(custom_event_args),
                        custom_event_args)
          .ToLocalChecked();

  v8::Local<v8::Function> dispatch_event =
      GetProperty(context, context->Global(), "dispatchEvent")
          .ToLocalChecked()
          .As<v8::Function>();

  v8::Local<v8::Value> dispatch_event_args[] = {std::move(custom_event_value)};

  dispatch_event
      ->Call(context, context->Global(), ABSL_ARRAYSIZE(dispatch_event_args),
             dispatch_event_args)
      .ToLocalChecked();
}

const std::string& JSEthereumProvider::GetBraveWalletImage() {
  if (!brave_wallet_image_) {
    brave_wallet_image_ =
        LoadImageResourceAsDataUrl(IDR_BRAVE_WALLET_PROVIDER_ICON);
  }
  return brave_wallet_image_.value();
}

}  // namespace brave_wallet
