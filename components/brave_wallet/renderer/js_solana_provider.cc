/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_solana_provider.h"

#include <tuple>
#include <utility>

#include "base/notreached.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "brave/components/brave_wallet/renderer/v8_helper.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8-microtask-queue.h"

namespace brave_wallet {

JSSolanaProvider::JSSolanaProvider(bool use_native_wallet,
                                   content::RenderFrame* render_frame)
    : use_native_wallet_(use_native_wallet),
      render_frame_(render_frame),
      v8_value_converter_(content::V8ValueConverter::Create()) {
  EnsureConnected();
}
JSSolanaProvider::~JSSolanaProvider() = default;

gin::WrapperInfo JSSolanaProvider::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
std::unique_ptr<JSSolanaProvider> JSSolanaProvider::Install(
    bool use_native_wallet,
    content::RenderFrame* render_frame,
    v8::Local<v8::Context> context) {
  std::unique_ptr<JSSolanaProvider> js_solana_provider(
      new JSSolanaProvider(use_native_wallet, render_frame));
  v8::Isolate* isolate = context->GetIsolate();
  gin::Handle<JSSolanaProvider> provider =
      gin::CreateHandle(isolate, js_solana_provider.get());
  if (provider.IsEmpty())
    return nullptr;
  v8::Local<v8::Object> global = context->Global();
  global->Set(context, gin::StringToSymbol(isolate, "solana"), provider.ToV8())
      .Check();
  return js_solana_provider;
}

gin::ObjectTemplateBuilder JSSolanaProvider::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<JSSolanaProvider>::GetObjectTemplateBuilder(isolate)
      .SetProperty("isPhantom", &JSSolanaProvider::GetIsPhantom)
      .SetProperty("isConnected", &JSSolanaProvider::GetIsConnected)
      .SetMethod("connect", &JSSolanaProvider::Connect)
      .SetMethod("disconnect", &JSSolanaProvider::Disconnect)
      .SetMethod("signAndSendTransaction",
                 &JSSolanaProvider::SignAndSendTransaction)
      .SetMethod("signMessage", &JSSolanaProvider::SignMessage)
      .SetMethod("request", &JSSolanaProvider::Request)
      // Deprecated
      .SetMethod("signTransaction", &JSSolanaProvider::SignTransaction)
      // Deprecated
      .SetMethod("signAllTransaction", &JSSolanaProvider::SignAllTransaction);
}

const char* JSSolanaProvider::GetTypeName() {
  return "JSSolanaProvider";
}

bool JSSolanaProvider::EnsureConnected() {
  if (use_native_wallet_ && !solana_provider_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        solana_provider_.BindNewPipeAndPassReceiver());
    solana_provider_.set_disconnect_handler(base::BindOnce(
        &JSSolanaProvider::OnRemoteDisconnect, weak_ptr_factory_.GetWeakPtr()));
  }

  return solana_provider_.is_bound();
}

void JSSolanaProvider::OnRemoteDisconnect() {
  solana_provider_.reset();
  EnsureConnected();
}

bool JSSolanaProvider::GetIsPhantom(gin::Arguments* arguments) {
  return true;
}

bool JSSolanaProvider::GetIsConnected(gin::Arguments* arguments) {
  bool is_connected = false;
  if (!solana_provider_->IsConnected(&is_connected)) {
    return false;
  }
  return is_connected;
}

v8::Local<v8::Promise> JSSolanaProvider::Connect(gin::Arguments* arguments) {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }

  // Get base::Value arg to pass
  absl::optional<base::Value> arg = absl::nullopt;
  v8::Local<v8::Value> v8_arg;
  if (arguments->Length() > 1 ||
      (arguments->Length() == 1 && !arguments->GetNext(&v8_arg))) {
    arguments->ThrowError();
    return v8::Local<v8::Promise>();
  }
  if (!v8_arg.IsEmpty()) {
    std::unique_ptr<base::Value> arg_value =
        v8_value_converter_->FromV8Value(v8_arg, isolate->GetCurrentContext());
    if (!arg_value || !arg_value->is_dict()) {
      arguments->ThrowError();
      return v8::Local<v8::Promise>();
    }
    arg = std::move(*arg_value);
  }

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  solana_provider_->Connect(
      std::move(arg),
      base::BindOnce(&JSSolanaProvider::OnConnect,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver.ToLocalChecked()->GetPromise();
}

v8::Local<v8::Promise> JSSolanaProvider::Disconnect(gin::Arguments* arguments) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }

  solana_provider_->Disconnect();

  std::ignore = resolver.ToLocalChecked()->Resolve(isolate->GetCurrentContext(),
                                                   v8::Undefined(isolate));

  FireEvent(kDisconnectEvent, std::vector<v8::Local<v8::Value>>());
  return resolver.ToLocalChecked()->GetPromise();
}

v8::Local<v8::Promise> JSSolanaProvider::SignAndSendTransaction(
    gin::Arguments* arguments) {
  NOTIMPLEMENTED();
  return v8::Local<v8::Promise>();
}

v8::Local<v8::Promise> JSSolanaProvider::SignMessage(
    gin::Arguments* arguments) {
  NOTIMPLEMENTED();
  return v8::Local<v8::Promise>();
}

v8::Local<v8::Promise> JSSolanaProvider::Request(gin::Arguments* arguments) {
  NOTIMPLEMENTED();
  return v8::Local<v8::Promise>();
}

// Deprecated
v8::Local<v8::Promise> JSSolanaProvider::SignTransaction(
    gin::Arguments* arguments) {
  NOTIMPLEMENTED();
  return v8::Local<v8::Promise>();
}

// Deprecated
v8::Local<v8::Promise> JSSolanaProvider::SignAllTransaction(
    gin::Arguments* arguments) {
  NOTIMPLEMENTED();
  return v8::Local<v8::Promise>();
}

void JSSolanaProvider::FireEvent(
    const std::string& event,
    std::vector<v8::Local<v8::Value>>&& event_args) {
  v8::Local<v8::Context> context =
      render_frame_->GetWebFrame()->MainWorldScriptContext();
  std::vector<v8::Local<v8::Value>> args;
  const base::Value event_value(event);
  args.push_back(v8_value_converter_->ToV8Value(&event_value, context));
  args.insert(args.end(), event_args.begin(), event_args.end());
  CallMethodOfObject(render_frame_->GetWebFrame(), u"solana", u"emit",
                     std::move(args));
}

void JSSolanaProvider::OnConnect(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    mojom::SolanaProviderError error,
    const std::string& error_message,
    const std::string& public_key) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Local<v8::Value> result;
  v8::Local<v8::Value> v8_public_key;
  if (error == mojom::SolanaProviderError::kSuccess) {
    // use @solana/web3.js and create publicKey from base58 string
    const base::Value public_key_value(public_key);
    std::vector<v8::Local<v8::Value>> args;
    args.push_back(v8_value_converter_->ToV8Value(&public_key_value, context));

    v8::MaybeLocal<v8::Value> public_key_result =
        CallMethodOfObject(render_frame_->GetWebFrame(), u"solana",
                           u"createPublickey", std::move(args));
    result = public_key_result.ToLocalChecked();
    // for connect event
    CHECK(GetProperty(context, result, u"publicKey").ToLocal(&v8_public_key));
  } else {
    std::unique_ptr<base::Value> formed_response =
        GetProviderErrorDictionary(error, error_message);
    result = v8_value_converter_->ToV8Value(formed_response.get(), context);
  }

  SendResponse(std::move(global_context), std::move(promise_resolver), isolate,
               std::move(result),
               error == mojom::SolanaProviderError::kSuccess);

  std::vector<v8::Local<v8::Value>> args;
  args.push_back(std::move(v8_public_key));
  FireEvent(kConnectEvent, std::move(args));
}

void JSSolanaProvider::SendResponse(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    v8::Local<v8::Value> response,
    bool success) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  if (success) {
    std::ignore = resolver->Resolve(context, response);
  } else {
    std::ignore = resolver->Reject(context, response);
  }
}

}  // namespace brave_wallet
