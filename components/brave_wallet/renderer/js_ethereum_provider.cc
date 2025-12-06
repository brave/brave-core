/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_ethereum_provider.h"

#include <limits>
#include <tuple>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/uuid.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "brave/components/brave_wallet/renderer/resource_helper.h"
#include "brave/components/brave_wallet/renderer/v8_helper.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_script_generated.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/common/isolated_world_ids.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "third_party/abseil-cpp/absl/base/macros.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "ui/base/l10n/l10n_util.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-proxy.h"

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

constexpr char kEthereumChainChangedEvent[] = "chainChanged";
constexpr char kEthereumAccountsChangedEvent[] = "accountsChanged";
constexpr char kEthereumMessageEvent[] = "message";

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
  self_ = this;
}

JSEthereumProvider::~JSEthereumProvider() = default;

void JSEthereumProvider::OnDestruct() {
  self_.Clear();
}

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

void JSEthereumProvider::DidFinishLoad() {
  if (script_context_released_) {
    return;
  }

  // These used to be called synchronously by `JSEthereumProvider::Install`
  // which appeared to cause rare crashes with certain extensions' behavior. See
  // https://github.com/brave/brave-browser/issues/45694 for details.
  BindRequestProviderListener();
  AnnounceProvider();
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

  JSEthereumProvider* provider =
      cppgc::MakeGarbageCollected<JSEthereumProvider>(
          isolate->GetCppHeap()->GetAllocationHandle(), render_frame);
  v8::Local<v8::Object> provider_object =
      provider->GetWrapper(isolate).ToLocalChecked();

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
  if (install_ethereum_provider) {
    v8::Local<v8::Value> ethereum_value =
        global->Get(context, gin::StringToV8(isolate, kEthereum))
            .ToLocalChecked();
    if (ethereum_value->IsUndefined()) {
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
}

bool JSEthereumProvider::GetIsBraveWallet() {
  return true;
}

bool JSEthereumProvider::GetIsMetaMask() {
  return true;
}

JSEthereumProvider::MetaMask::MetaMask(content::RenderFrame* render_frame)
    : render_frame_(render_frame) {}
JSEthereumProvider::MetaMask::~MetaMask() = default;

gin::ObjectTemplateBuilder
JSEthereumProvider::MetaMask::GetObjectTemplateBuilder(v8::Isolate* isolate) {
  return gin::Wrappable<MetaMask>::GetObjectTemplateBuilder(isolate).SetMethod(
      kIsUnlocked, &JSEthereumProvider::MetaMask::IsUnlocked);
}

const gin::WrapperInfo* JSEthereumProvider::MetaMask::wrapper_info() const {
  return &kWrapperInfo;
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
  MetaMask* metamask = cppgc::MakeGarbageCollected<MetaMask>(
      isolate->GetCppHeap()->GetAllocationHandle(), render_frame());
  v8::Local<v8::Object> object = metamask->GetWrapper(isolate).ToLocalChecked();

  SetOwnPropertyWritable(isolate->GetCurrentContext(), object,
                         gin::StringToV8(isolate, kIsUnlocked), false);
  return object;
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
    uint64_t network_version = (uint64_t)chain_id_uint256;
    return gin::StringToV8(isolate, base::NumberToString(network_version));
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

const gin::WrapperInfo* JSEthereumProvider::wrapper_info() const {
  return &kWrapperInfo;
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

  base::Value::List params;
  if (args->Length() > 1) {
    v8::Local<v8::Value> arg2;
    if (!args->GetNext(&arg2)) {
      args->ThrowError();
      return v8::Local<v8::Promise>();
    }
    auto arg_params = content::V8ValueConverter::Create()->FromV8Value(
        arg2, isolate->GetCurrentContext());
    if (!arg_params || !arg_params->is_list()) {
      args->ThrowError();
      return v8::Local<v8::Promise>();
    }
    params = std::move(arg_params->GetList());
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
      method, std::move(params),
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

  if (!input_value) {
    args->ThrowError();
    return;
  }

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

  if (!input_value) {
    return v8::Local<v8::Promise>();
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
    mojom::EthereumProviderResponsePtr response) {
  if (response->update_bind_js_properties) {
    first_allowed_account_ = response->first_allowed_account;
  }
  SendResponse(std::move(response->id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               std::move(response->formed_response), !response->reject);
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

  FireEvent(kEthereumChainChangedEvent, base::Value(chain_id));
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
  FireEvent(kEthereumAccountsChangedEvent, event_args);
}

void JSEthereumProvider::MessageEvent(const std::string& subscription_id,
                                      base::Value result) {
  base::Value::Dict event_args;
  base::Value::Dict data;
  data.Set("subscription", subscription_id);
  data.Set("result", std::move(result));
  event_args.Set("type", "eth_subscription");
  event_args.Set("data", std::move(data));
  FireEvent(kEthereumMessageEvent, event_args);
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

  if (context.IsEmpty()) {
    return;
  }

  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Context::Scope context_scope(context);

  auto on_provider_requested = gin::CreateFunctionTemplate(
      isolate, base::BindRepeating(&JSEthereumProvider::OnProviderRequested,
                                   weak_ptr_factory_.GetWeakPtr()));
  auto function_instance =
      on_provider_requested->GetFunction(context).ToLocalChecked();

  std::vector<v8::Local<v8::Value>> args;
  args.push_back(content::V8ValueConverter::Create()->ToV8Value(
      base::Value("eip6963:requestProvider"), context));
  args.push_back(std::move(function_instance));

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

  if (context.IsEmpty()) {
    return;
  }

  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Context::Scope context_scope(context);

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
           .FromMaybe(false)) {
    return;
  }

  if (detail
          ->Set(context,
                content::V8ValueConverter::Create()->ToV8Value(
                    base::Value("info"), context),
                std::move(info_object))
          .IsNothing()) {
    return;
  }

  v8::Local<v8::Value> provider;
  if (!GetProperty(context, context->Global(), kBraveEthereum)
           .ToLocal(&provider)) {
    return;
  }

  if (detail
          ->Set(context,
                content::V8ValueConverter::Create()->ToV8Value(
                    base::Value("provider"), context),
                provider)
          .IsNothing()) {
    return;
  }

  if (detail
          ->SetIntegrityLevel(isolate->GetCurrentContext(),
                              v8::IntegrityLevel::kFrozen)
          .IsNothing()) {
    return;
  }

  auto event_content = v8::Object::New(isolate);
  if (event_content
          ->Set(context,
                content::V8ValueConverter::Create()->ToV8Value(
                    base::Value("detail"), context),
                std::move(detail))
          .IsNothing()) {
    return;
  }

  v8::Local<v8::Value> custom_event_function;
  if (!GetProperty(context, context->Global(), "CustomEvent")
           .ToLocal(&custom_event_function) ||
      !custom_event_function->IsFunction()) {
    return;
  }

  v8::Local<v8::Value> custom_event_args[] = {
      content::V8ValueConverter::Create()->ToV8Value(
          base::Value("eip6963:announceProvider"), context),
      std::move(event_content)};

  v8::Local<v8::Value> custom_event_value;
  if (!v8::Local<v8::Function>::Cast(custom_event_function)
           ->NewInstance(context, ABSL_ARRAYSIZE(custom_event_args),
                         custom_event_args)
           .ToLocal(&custom_event_value)) {
    return;
  }

  v8::Local<v8::Value> dispatch_event_function;
  if (!GetProperty(context, context->Global(), "dispatchEvent")
           .ToLocal(&dispatch_event_function) ||
      !dispatch_event_function->IsFunction()) {
    return;
  }

  v8::Local<v8::Value> dispatch_event_args[] = {std::move(custom_event_value)};

  std::ignore =
      v8::Local<v8::Function>::Cast(dispatch_event_function)
          ->Call(context, context->Global(),
                 ABSL_ARRAYSIZE(dispatch_event_args), dispatch_event_args);
}

const std::string& JSEthereumProvider::GetBraveWalletImage() {
  if (!brave_wallet_image_) {
    brave_wallet_image_ =
        "data:image/png;base64," +
        base::Base64Encode(LoadDataResource(IDR_BRAVE_WALLET_PROVIDER_ICON));
  }
  return brave_wallet_image_.value();
}

}  // namespace brave_wallet
