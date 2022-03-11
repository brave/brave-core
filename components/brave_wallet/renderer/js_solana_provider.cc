/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_solana_provider.h"

#include <tuple>
#include <utility>

#include "base/notreached.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "brave/components/brave_wallet/renderer/v8_helper.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/array_buffer.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8-microtask-queue.h"

namespace brave_wallet {

JSSolanaProvider::JSSolanaProvider(bool use_native_wallet,
                                   content::RenderFrame* render_frame)
    : use_native_wallet_(use_native_wallet),
      render_frame_(render_frame),
      v8_value_converter_(content::V8ValueConverter::Create()) {
  EnsureConnected();
  v8_value_converter_->SetStrategy(&strategy_);
}
JSSolanaProvider::~JSSolanaProvider() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

gin::WrapperInfo JSSolanaProvider::kWrapperInfo = {gin::kEmbedderNativeGin};

// Convert Uint8Array to base58 encoded string
bool JSSolanaProvider::V8ConverterStrategy::FromV8ArrayBuffer(
    v8::Local<v8::Object> value,
    std::unique_ptr<base::Value>* out,
    v8::Isolate* isolate) {
  if (!value->IsTypedArray()) {
    return false;
  }
  std::vector<uint8_t> bytes;
  char* data = NULL;
  size_t data_length = 0;
  gin::ArrayBufferView view;
  if (gin::ConvertFromV8(isolate, value.As<v8::ArrayBufferView>(), &view)) {
    data = reinterpret_cast<char*>(view.bytes());
    data_length = view.num_bytes();
    bytes.assign(data, data + data_length);
  }
  if (!bytes.size())
    return false;
  std::unique_ptr<base::Value> new_value =
      std::make_unique<base::Value>(Base58Encode(bytes));
  *out = std::move(new_value);

  return true;
}

// static
std::unique_ptr<JSSolanaProvider> JSSolanaProvider::Install(
    bool use_native_wallet,
    content::RenderFrame* render_frame,
    v8::Local<v8::Context> context) {
  std::unique_ptr<JSSolanaProvider> js_solana_provider(
      new JSSolanaProvider(use_native_wallet, render_frame));
  if (!js_solana_provider->Init(context))
    return nullptr;

  return js_solana_provider;
}

bool JSSolanaProvider::Init(v8::Local<v8::Context> context) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Value> solana_value;
  if (!global->Get(context, gin::StringToV8(isolate, "solana"))
           .ToLocal(&solana_value) ||
      !solana_value->IsObject()) {
    gin::Handle<JSSolanaProvider> provider = gin::CreateHandle(isolate, this);
    if (provider.IsEmpty())
      return false;
    global
        ->Set(context, gin::StringToSymbol(isolate, "solana"), provider.ToV8())
        .Check();
  } else {
    render_frame_->GetWebFrame()->AddMessageToConsole(
        blink::WebConsoleMessage(blink::mojom::ConsoleMessageLevel::kWarning,
                                 "Brave Wallet will not insert window.solana "
                                 "because it already exists!"));
  }
  return true;
}

gin::ObjectTemplateBuilder JSSolanaProvider::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<JSSolanaProvider>::GetObjectTemplateBuilder(isolate)
      .SetProperty("isPhantom", &JSSolanaProvider::GetIsPhantom)
      .SetProperty("isConnected", &JSSolanaProvider::GetIsConnected)
      .SetProperty("publicKey", &JSSolanaProvider::GetPublicKey)
      .SetMethod("connect", &JSSolanaProvider::Connect)
      .SetMethod("disconnect", &JSSolanaProvider::Disconnect)
      .SetMethod("signAndSendTransaction",
                 &JSSolanaProvider::SignAndSendTransaction)
      .SetMethod("signMessage", &JSSolanaProvider::SignMessage)
      .SetMethod("request", &JSSolanaProvider::Request)
      // Deprecated
      .SetMethod("signTransaction", &JSSolanaProvider::SignTransaction)
      // Deprecated
      .SetMethod("signAllTransactions", &JSSolanaProvider::SignAllTransactions);
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

v8::Local<v8::Value> JSSolanaProvider::GetPublicKey(gin::Arguments* arguments) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  std::string public_key;
  if (!solana_provider_->GetPublicKey(&public_key) || public_key.empty())
    return v8::Null(isolate);
  const base::Value public_key_value(public_key);
  std::vector<v8::Local<v8::Value>> args;
  args.push_back(v8_value_converter_->ToV8Value(&public_key_value, context));

  v8::MaybeLocal<v8::Value> public_key_result =
      CallMethodOfObject(render_frame_->GetWebFrame(), u"solana",
                         u"createPublickey", std::move(args));
  v8::Local<v8::Value> v8_public_key;
  CHECK(GetProperty(context, public_key_result.ToLocalChecked(), u"publicKey")
            .ToLocal(&v8_public_key));
  return v8_public_key;
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
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();
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
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }
  v8::Local<v8::Value> transaction;
  if (arguments->Length() != 1 || !arguments->GetNext(&transaction)) {
    arguments->ThrowError();
    return v8::Local<v8::Promise>();
  }

  absl::optional<std::string> serialized_message =
      GetSerializedMessage(transaction);
  if (!serialized_message)
    return v8::Local<v8::Promise>();

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  solana_provider_->SignAndSendTransaction(
      *serialized_message,
      base::BindOnce(&JSSolanaProvider::OnSignAndSendTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver.ToLocalChecked()->GetPromise();
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
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }
  v8::Local<v8::Value> transaction;
  if (arguments->Length() != 1 || !arguments->GetNext(&transaction)) {
    arguments->ThrowError();
    return v8::Local<v8::Promise>();
  }

  absl::optional<std::string> serialized_message =
      GetSerializedMessage(transaction);
  if (!serialized_message)
    return v8::Local<v8::Promise>();

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  solana_provider_->SignTransaction(
      *serialized_message,
      base::BindOnce(&JSSolanaProvider::OnSignTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver.ToLocalChecked()->GetPromise();
}

// Deprecated
v8::Local<v8::Promise> JSSolanaProvider::SignAllTransactions(
    gin::Arguments* arguments) {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }
  v8::Local<v8::Value> transactions;
  if (arguments->Length() != 1 || !arguments->GetNext(&transactions) ||
      !transactions->IsArray()) {
    arguments->ThrowError();
    return v8::Local<v8::Promise>();
  }
  std::vector<std::string> serialized_messages;
  v8::Local<v8::Array> transactions_array = transactions.As<v8::Array>();
  uint32_t transactions_count = transactions_array->Length();
  serialized_messages.reserve(transactions_count);
  for (uint32_t i = 0; i < transactions_count; ++i) {
    absl::optional<std::string> serialized_message = GetSerializedMessage(
        transactions_array->Get(context, i).ToLocalChecked());
    if (serialized_message)
      serialized_messages.push_back(*serialized_message);
  }

  auto global_context(v8::Global<v8::Context>(isolate, context));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  solana_provider_->SignAllTransactions(
      serialized_messages,
      base::BindOnce(&JSSolanaProvider::OnSignAllTransactions,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver.ToLocalChecked()->GetPromise();
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

void JSSolanaProvider::OnSignAndSendTransaction(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    mojom::SolanaProviderError error,
    const std::string& error_message,
    base::Value result) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Local<v8::Value> v8_result;
  v8::Local<v8::Value> v8_serialized_tx;
  if (error == mojom::SolanaProviderError::kSuccess) {
    const base::Value result_value(std::move(result));
    v8_result = v8_value_converter_->ToV8Value(&result_value, context);
  } else {
    std::unique_ptr<base::Value> formed_response =
        GetProviderErrorDictionary(error, error_message);
    v8_result = v8_value_converter_->ToV8Value(formed_response.get(), context);
  }

  SendResponse(std::move(global_context), std::move(promise_resolver), isolate,
               std::move(v8_result),
               error == mojom::SolanaProviderError::kSuccess);
}

void JSSolanaProvider::OnSignTransaction(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    mojom::SolanaProviderError error,
    const std::string& error_message,
    const std::vector<uint8_t>& serialized_tx) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Local<v8::Value> result;
  v8::Local<v8::Value> v8_serialized_tx;
  if (error == mojom::SolanaProviderError::kSuccess) {
    // use @solana/web3.js and create Transaction from serialized tx
    const base::Value serialized_tx_value(serialized_tx);
    std::vector<v8::Local<v8::Value>> args;
    args.push_back(
        v8_value_converter_->ToV8Value(&serialized_tx_value, context));

    v8::MaybeLocal<v8::Value> transaction_result =
        CallMethodOfObject(render_frame_->GetWebFrame(), u"solana",
                           u"createTransaction", std::move(args));
    result = transaction_result.ToLocalChecked();
  } else {
    std::unique_ptr<base::Value> formed_response =
        GetProviderErrorDictionary(error, error_message);
    result = v8_value_converter_->ToV8Value(formed_response.get(), context);
  }

  SendResponse(std::move(global_context), std::move(promise_resolver), isolate,
               std::move(result),
               error == mojom::SolanaProviderError::kSuccess);
}

void JSSolanaProvider::OnSignAllTransactions(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    mojom::SolanaProviderError error,
    const std::string& error_message,
    const std::vector<std::vector<uint8_t>>& serialized_txs) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Value> result;
  if (error == mojom::SolanaProviderError::kSuccess) {
    // create new scoped context
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);

    size_t serialized_txs_length = serialized_txs.size();
    v8::Local<v8::Array> tx_array =
        v8::Array::New(context->GetIsolate(), serialized_txs_length);
    for (size_t i = 0; i < serialized_txs_length; ++i) {
      const base::Value serialized_tx_value(serialized_txs[i]);
      std::vector<v8::Local<v8::Value>> args;
      args.push_back(
          v8_value_converter_->ToV8Value(&serialized_tx_value, context));

      v8::MaybeLocal<v8::Value> transaction_result =
          CallMethodOfObject(render_frame_->GetWebFrame(), u"solana",
                             u"createTransaction", std::move(args));
      v8::Maybe<bool> success = tx_array->CreateDataProperty(
          context, i, transaction_result.ToLocalChecked());
      CHECK(success.ToChecked());
    }
    result = tx_array;
  } else {
    std::unique_ptr<base::Value> formed_response =
        GetProviderErrorDictionary(error, error_message);
    result = v8_value_converter_->ToV8Value(formed_response.get(),
                                            global_context.Get(isolate));
  }

  SendResponse(std::move(global_context), std::move(promise_resolver), isolate,
               std::move(result),
               error == mojom::SolanaProviderError::kSuccess);
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

absl::optional<std::string> JSSolanaProvider::GetSerializedMessage(
    v8::Local<v8::Value> transaction) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::MaybeLocal<v8::Value> serialized_msg = CallMethodOfObject(
      render_frame_->GetWebFrame(), transaction, u"serializeMessage",
      std::vector<v8::Local<v8::Value>>());
  // base58 encode Uint8Array which is defined in
  // V8ConverterStrategy::FromV8ArrayBuffer
  std::unique_ptr<base::Value> encoded_msg = v8_value_converter_->FromV8Value(
      serialized_msg.ToLocalChecked(), isolate->GetCurrentContext());
  if (!encoded_msg->is_string())
    return absl::nullopt;

  return encoded_msg->GetString();
}

}  // namespace brave_wallet
