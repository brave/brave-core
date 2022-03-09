/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_solana_provider.h"

#include <tuple>
#include <utility>
#include <vector>

#include "base/notreached.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-microtask-queue.h"

namespace brave_wallet {

namespace {

// TODO: move these to helper
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
v8::MaybeLocal<v8::Value> CallMethodOfObject(
    blink::WebLocalFrame* web_frame,
    const std::u16string& object_name,
    const std::u16string& method_name,
    std::vector<v8::Local<v8::Value>>&& args) {
  if (web_frame->IsProvisional())
    return v8::Local<v8::Value>();
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  v8::Local<v8::Context> context = web_frame->MainWorldScriptContext();
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(v8::Isolate::GetCurrent(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Value> object;
  v8::Local<v8::Value> method;
  if (!GetProperty(context, context->Global(), object_name).ToLocal(&object) ||
      !GetProperty(context, object, method_name).ToLocal(&method)) {
    return v8::Local<v8::Value>();
  }

  // Without the IsFunction test here JS blocking from content settings
  // will trigger a DCHECK crash.
  if (method.IsEmpty() || !method->IsFunction()) {
    return v8::Local<v8::Value>();
  }

  return web_frame->CallFunctionEvenIfScriptDisabled(
      v8::Local<v8::Function>::Cast(method), object,
      static_cast<int>(args.size()), args.data());
}

}  // namespace

JSSolanaProvider::JSSolanaProvider(bool use_native_wallet,
                                   content::RenderFrame* render_frame)
    : use_native_wallet_(use_native_wallet), render_frame_(render_frame) {
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
  NOTIMPLEMENTED();
  return false;
}

v8::Local<v8::Promise> JSSolanaProvider::Connect(gin::Arguments* arguments) {
  // TODO: support onlyIfTrusted
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
  solana_provider_->Connect(base::BindOnce(
      &JSSolanaProvider::OnConnect, weak_ptr_factory_.GetWeakPtr(),
      std::move(global_context), std::move(promise_resolver), isolate));

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

  // emit disconnect
  std::vector<v8::Local<v8::Value>> args;
  const base::Value event("disconnect");
  args.push_back(content::V8ValueConverter::Create()->ToV8Value(
      &event, isolate->GetCurrentContext()));
  CallMethodOfObject(render_frame_->GetWebFrame(), u"solana", u"emit",
                     std::move(args));
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
  std::unique_ptr<content::V8ValueConverter> v8_value_converter =
      content::V8ValueConverter::Create();
  if (error == mojom::SolanaProviderError::kSuccess) {
    // use @solana/web3.js and create publicKey from base58 string
    const base::Value public_key_value(public_key);
    std::vector<v8::Local<v8::Value>> args;
    args.push_back(v8_value_converter->ToV8Value(&public_key_value, context));

    v8::MaybeLocal<v8::Value> public_key_result =
        CallMethodOfObject(render_frame_->GetWebFrame(), u"solana",
                           u"createPublickey", std::move(args));
    result = public_key_result.ToLocalChecked();
    // for connect event
    CHECK(GetProperty(context, result, u"publicKey").ToLocal(&v8_public_key));
  } else {
    std::unique_ptr<base::Value> formed_response =
        GetProviderErrorDictionary(error, error_message);
    result = v8_value_converter->ToV8Value(formed_response.get(), context);
  }

  SendResponse(std::move(global_context), std::move(promise_resolver), isolate,
               std::move(result),
               error == mojom::SolanaProviderError::kSuccess);
  // emit connect
  std::vector<v8::Local<v8::Value>> args;
  const base::Value event("connect");
  args.push_back(v8_value_converter->ToV8Value(&event, context));
  args.push_back(std::move(v8_public_key));
  CallMethodOfObject(render_frame_->GetWebFrame(), u"solana", u"emit",
                     std::move(args));
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
