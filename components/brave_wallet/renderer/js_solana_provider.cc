/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_solana_provider.h"

#include <tuple>
#include <utility>

#include "base/no_destructor.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "brave/components/brave_wallet/renderer/resource_helper.h"
#include "brave/components/brave_wallet/renderer/v8_helper.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_script_generated.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/array_buffer.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-typed-array.h"

namespace brave_wallet {

namespace {

static base::NoDestructor<std::string> g_provider_script("");

}  // namespace

JSSolanaProvider::JSSolanaProvider(content::RenderFrame* render_frame)
    : render_frame_(render_frame),
      v8_value_converter_(content::V8ValueConverter::Create()) {
  if (g_provider_script->empty()) {
    *g_provider_script = LoadDataResource(
        IDR_BRAVE_WALLET_SCRIPT_SOLANA_PROVIDER_SCRIPT_BUNDLE_JS);
  }
  EnsureConnected();
  v8_value_converter_->SetStrategy(&strategy_);
}

JSSolanaProvider::~JSSolanaProvider() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

gin::WrapperInfo JSSolanaProvider::kWrapperInfo = {gin::kEmbedderNativeGin};

// Convert Uint8Array to blob base::Value
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
      std::make_unique<base::Value>(bytes);
  *out = std::move(new_value);

  return true;
}

// static
void JSSolanaProvider::Install(bool allow_overwrite_window_solana,
                               bool is_main_world,
                               content::RenderFrame* render_frame,
                               v8::Local<v8::Context> context) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks_scope(
      isolate, v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Context::Scope context_scope(context);
  if (is_main_world) {
    v8::Local<v8::Object> global = context->Global();
    v8::Local<v8::Value> solana_value;
    if (!global->Get(context, gin::StringToV8(isolate, "solana"))
             .ToLocal(&solana_value) ||
        !solana_value->IsObject()) {
      gin::Handle<JSSolanaProvider> provider =
          gin::CreateHandle(isolate, new JSSolanaProvider(render_frame));
      CHECK(!provider.IsEmpty());
      global
          ->Set(context, gin::StringToSymbol(isolate, "solana"),
                provider.ToV8())
          .Check();
    } else {
      render_frame->GetWebFrame()->AddMessageToConsole(
          blink::WebConsoleMessage(blink::mojom::ConsoleMessageLevel::kWarning,
                                   "Brave Wallet will not insert window.solana "
                                   "because it already exists!"));
    }
  }
  blink::WebLocalFrame* web_frame = render_frame->GetWebFrame();
  if (!allow_overwrite_window_solana) {
    SetProviderNonWritable(web_frame, "solana");
  }
  if (is_main_world) {
    ExecuteScript(web_frame, *g_provider_script);
  }
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

void JSSolanaProvider::AccountChangedEvent(
    const absl::optional<std::string>& account) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame_->GetWebFrame()->MainWorldScriptContext();
  std::vector<v8::Local<v8::Value>> args;
  if (!account) {
    // emits Null
    args.push_back(v8::Null(isolate));
  } else {
    // emits solanaWeb3.PublicKey
    v8::Local<v8::Value> v8_public_key;
    CHECK(GetProperty(context, CreatePublicKey(context, *account), u"publicKey")
              .ToLocal(&v8_public_key));
    args.push_back(std::move(v8_public_key));
  }
  FireEvent(solana::kAccountChangedEvent, std::move(args));
}

bool JSSolanaProvider::EnsureConnected() {
  if (!solana_provider_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        solana_provider_.BindNewPipeAndPassReceiver());
    solana_provider_->Init(receiver_.BindNewPipeAndPassRemote());
    solana_provider_.set_disconnect_handler(base::BindOnce(
        &JSSolanaProvider::OnRemoteDisconnect, weak_ptr_factory_.GetWeakPtr()));
  }

  return solana_provider_.is_bound();
}

void JSSolanaProvider::OnRemoteDisconnect() {
  solana_provider_.reset();
  receiver_.reset();
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

  v8::Local<v8::Value> v8_public_key;
  CHECK(GetProperty(context, CreatePublicKey(context, public_key), u"publicKey")
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

  // Get base::Value arg to pass and ignore extra parameters
  absl::optional<base::Value> arg = absl::nullopt;
  v8::Local<v8::Value> v8_arg;
  if (arguments->Length() >= 1 && !arguments->GetNext(&v8_arg)) {
    arguments->ThrowTypeError(
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return v8::Local<v8::Promise>();
  }
  if (!v8_arg.IsEmpty()) {
    std::unique_ptr<base::Value> arg_value =
        v8_value_converter_->FromV8Value(v8_arg, isolate->GetCurrentContext());
    if (!arg_value || !arg_value->is_dict()) {
      arguments->ThrowTypeError(
          l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
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
  if (arguments->Length() < 1 || !arguments->GetNext(&transaction)) {
    arguments->ThrowTypeError(
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return v8::Local<v8::Promise>();
  }

  absl::optional<std::string> serialized_message =
      GetSerializedMessage(transaction);
  if (!serialized_message) {
    arguments->ThrowTypeError(
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return v8::Local<v8::Promise>();
  }

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
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }
  v8::Local<v8::Value> message;
  if (arguments->Length() < 1 || !arguments->GetNext(&message)) {
    arguments->ThrowTypeError(
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return v8::Local<v8::Promise>();
  }

  std::unique_ptr<base::Value> blob_msg =
      v8_value_converter_->FromV8Value(message, isolate->GetCurrentContext());
  if (!blob_msg->is_blob()) {
    arguments->ThrowTypeError(
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return v8::Local<v8::Promise>();
  }

  absl::optional<std::string> display_str = absl::nullopt;
  v8::Local<v8::Value> display;
  if (arguments->GetNext(&display)) {
    std::unique_ptr<base::Value> display_value =
        v8_value_converter_->FromV8Value(display, isolate->GetCurrentContext());
    if (display_value->is_string())
      display_str = display_value->GetString();
  }

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  solana_provider_->SignMessage(
      blob_msg->GetBlob(), display_str,
      base::BindOnce(&JSSolanaProvider::OnSignMessage,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver.ToLocalChecked()->GetPromise();
}

v8::Local<v8::Promise> JSSolanaProvider::Request(gin::Arguments* arguments) {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }
  v8::Local<v8::Value> arg;
  if (arguments->Length() < 1 || !arguments->GetNext(&arg)) {
    arguments->ThrowTypeError(
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return v8::Local<v8::Promise>();
  }

  std::unique_ptr<base::Value> arg_value =
      v8_value_converter_->FromV8Value(arg, isolate->GetCurrentContext());
  if (!arg_value->is_dict()) {
    arguments->ThrowTypeError(
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return v8::Local<v8::Promise>();
  }
  // Passing method to OnRequest in case it needs special handling, ex. connect
  // which requires us to construct a solanaWeb3.PublicKey object which can only
  // be done on renderer side.
  const std::string* method = arg_value->FindStringKey("method");
  if (!method) {
    arguments->ThrowTypeError(
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return v8::Local<v8::Promise>();
  }

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  solana_provider_->Request(
      std::move(*arg_value),
      base::BindOnce(&JSSolanaProvider::OnRequest,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate, *method));

  return resolver.ToLocalChecked()->GetPromise();
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
  if (arguments->Length() < 1 || !arguments->GetNext(&transaction)) {
    arguments->ThrowTypeError(
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return v8::Local<v8::Promise>();
  }

  absl::optional<std::string> serialized_message =
      GetSerializedMessage(transaction);
  if (!serialized_message) {
    arguments->ThrowTypeError(
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return v8::Local<v8::Promise>();
  }

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
  if (arguments->Length() < 1 || !arguments->GetNext(&transactions) ||
      !transactions->IsArray()) {
    arguments->ThrowTypeError(
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return v8::Local<v8::Promise>();
  }
  std::vector<std::string> serialized_messages;
  v8::Local<v8::Array> transactions_array = transactions.As<v8::Array>();
  uint32_t transactions_count = transactions_array->Length();
  serialized_messages.reserve(transactions_count);
  for (uint32_t i = 0; i < transactions_count; ++i) {
    absl::optional<std::string> serialized_message = GetSerializedMessage(
        transactions_array->Get(context, i).ToLocalChecked());
    if (!serialized_message) {
      arguments->ThrowTypeError(
          l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
      return v8::Local<v8::Promise>();
    }

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
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Value> result;
  if (error == mojom::SolanaProviderError::kSuccess) {
    result = CreatePublicKey(context, public_key);
  } else {
    std::unique_ptr<base::Value> formed_response =
        GetSolanaProviderErrorDictionary(error, error_message);
    result = v8_value_converter_->ToV8Value(formed_response.get(), context);
  }

  SendResponse(std::move(global_context), std::move(promise_resolver), isolate,
               result, error == mojom::SolanaProviderError::kSuccess);
  if (error == mojom::SolanaProviderError::kSuccess) {
    v8::Local<v8::Value> v8_public_key;
    CHECK(
        GetProperty(context, CreatePublicKey(context, public_key), u"publicKey")
            .ToLocal(&v8_public_key));
    std::vector<v8::Local<v8::Value>> args;
    args.push_back(std::move(v8_public_key));
    FireEvent(kConnectEvent, std::move(args));
  }
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
  if (error == mojom::SolanaProviderError::kSuccess) {
    // const base::Value result_value(std::move(result));
    v8_result = v8_value_converter_->ToV8Value(&result, context);
  } else {
    std::unique_ptr<base::Value> formed_response =
        GetSolanaProviderErrorDictionary(error, error_message);
    v8_result = v8_value_converter_->ToV8Value(formed_response.get(), context);
  }

  SendResponse(std::move(global_context), std::move(promise_resolver), isolate,
               v8_result, error == mojom::SolanaProviderError::kSuccess);
}

void JSSolanaProvider::OnSignMessage(
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
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Value> v8_result;
  if (error == mojom::SolanaProviderError::kSuccess) {
    const std::string* public_key = result.FindStringKey("publicKey");
    const std::string* signature = result.FindStringKey("signature");
    DCHECK(public_key && signature);
    v8::Local<v8::Value> v8_public_key;
    CHECK(GetProperty(context, CreatePublicKey(context, *public_key),
                      u"publicKey")
              .ToLocal(&v8_public_key));
    std::vector<uint8_t> signature_bytes(kSolanaSignatureSize);
    CHECK(Base58Decode(*signature, &signature_bytes, signature_bytes.size()));
    const base::Value signature_value(signature_bytes);
    v8::Local<v8::Value> v8_signature =
        v8_value_converter_->ToV8Value(&signature_value, context);
    // From ArraryBuffer to Uint8Array
    v8_signature =
        v8::Uint8Array::New(v8::Local<v8::ArrayBuffer>::Cast(v8_signature), 0,
                            (kSolanaSignatureSize));

    v8::Local<v8::Object> object = v8::Object::New(isolate);
    CHECK(CreateDataProperty(context, object, u"publicKey", v8_public_key)
              .ToChecked());
    CHECK(CreateDataProperty(context, object, u"signature", v8_signature)
              .ToChecked());
    v8_result = object;
  } else {
    std::unique_ptr<base::Value> formed_response =
        GetSolanaProviderErrorDictionary(error, error_message);
    v8_result = v8_value_converter_->ToV8Value(formed_response.get(), context);
  }

  SendResponse(std::move(global_context), std::move(promise_resolver), isolate,
               v8_result, error == mojom::SolanaProviderError::kSuccess);
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
  if (error == mojom::SolanaProviderError::kSuccess) {
    result = CreateTransaction(context, serialized_tx);
  } else {
    std::unique_ptr<base::Value> formed_response =
        GetSolanaProviderErrorDictionary(error, error_message);
    result = v8_value_converter_->ToV8Value(formed_response.get(), context);
  }

  SendResponse(std::move(global_context), std::move(promise_resolver), isolate,
               result, error == mojom::SolanaProviderError::kSuccess);
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
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Value> result;
  if (error == mojom::SolanaProviderError::kSuccess) {
    size_t serialized_txs_length = serialized_txs.size();
    v8::Local<v8::Array> tx_array =
        v8::Array::New(context->GetIsolate(), serialized_txs_length);
    for (size_t i = 0; i < serialized_txs_length; ++i) {
      v8::Local<v8::Value> transaction =
          CreateTransaction(context, serialized_txs[i]);

      CHECK(tx_array->CreateDataProperty(context, i, transaction).ToChecked());
    }
    result = tx_array;
  } else {
    std::unique_ptr<base::Value> formed_response =
        GetSolanaProviderErrorDictionary(error, error_message);
    result = v8_value_converter_->ToV8Value(formed_response.get(),
                                            global_context.Get(isolate));
  }

  SendResponse(std::move(global_context), std::move(promise_resolver), isolate,
               result, error == mojom::SolanaProviderError::kSuccess);
}

void JSSolanaProvider::OnRequest(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    const std::string& method,
    mojom::SolanaProviderError error,
    const std::string& error_message,
    base::Value result) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Value> v8_result;
  if (error == mojom::SolanaProviderError::kSuccess) {
    if (method == "connect") {
      const std::string* public_key = result.FindStringKey("publicKey");
      DCHECK(public_key);

      v8_result = CreatePublicKey(context, *public_key);
    } else {
      // Dictionary to object
      v8_result =
          v8_value_converter_->ToV8Value(&result, global_context.Get(isolate));
    }
  } else {
    std::unique_ptr<base::Value> formed_response =
        GetSolanaProviderErrorDictionary(error, error_message);
    v8_result = v8_value_converter_->ToV8Value(formed_response.get(),
                                               global_context.Get(isolate));
  }

  SendResponse(std::move(global_context), std::move(promise_resolver), isolate,
               v8_result, error == mojom::SolanaProviderError::kSuccess);
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
  if (serialized_msg.IsEmpty())
    return absl::nullopt;
  std::unique_ptr<base::Value> blob_value = v8_value_converter_->FromV8Value(
      serialized_msg.ToLocalChecked(), isolate->GetCurrentContext());
  if (!blob_value->is_blob())
    return absl::nullopt;

  return Base58Encode(blob_value->GetBlob());
}

v8::Local<v8::Value> JSSolanaProvider::CreatePublicKey(
    v8::Local<v8::Context> context,
    const std::string& base58_str) {
  const base::Value public_key_value(base58_str);
  std::vector<v8::Local<v8::Value>> args;
  args.push_back(v8_value_converter_->ToV8Value(&public_key_value, context));
  v8::MaybeLocal<v8::Value> public_key_result =
      CallMethodOfObject(render_frame_->GetWebFrame(), u"solana",
                         u"createPublickey", std::move(args));

  return public_key_result.ToLocalChecked();
}

v8::Local<v8::Value> JSSolanaProvider::CreateTransaction(
    v8::Local<v8::Context> context,
    const std::vector<uint8_t> serialized_tx) {
  const base::Value serialized_tx_value(serialized_tx);
  std::vector<v8::Local<v8::Value>> args;
  args.push_back(v8_value_converter_->ToV8Value(&serialized_tx_value, context));

  v8::MaybeLocal<v8::Value> transaction_result =
      CallMethodOfObject(render_frame_->GetWebFrame(), u"solana",
                         u"createTransaction", std::move(args));

  return transaction_result.ToLocalChecked();
}

}  // namespace brave_wallet
