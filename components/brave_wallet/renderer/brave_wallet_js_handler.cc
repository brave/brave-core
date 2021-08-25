/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_js_handler.h"

#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "brave/components/brave_wallet/common/web3_provider_utils.h"
#include "brave/components/brave_wallet/renderer/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_script_generated.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

static base::NoDestructor<std::string> g_provider_script("");

// Hardcode id to 1 as it is unused
const uint32_t kRequestId = 1;
const char kRequestJsonRPC[] = "2.0";

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
  std::vector<v8::Local<v8::Value>> args;
  for (auto const& argument : arguments.GetList()) {
    args.push_back(content::V8ValueConverter::Create()->ToV8Value(&argument,
                                                                  context));
  }

  web_frame->ExecuteMethodAndReturnValue(v8::Local<v8::Function>::Cast(method),
                                         object, static_cast<int>(args.size()),
                                         args.data());
}

void OnEthereumPermissionRequested(
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    v8::Global<v8::Context> context_old,
    bool success,
    const std::vector<std::string>& accounts) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  if (!success || accounts.empty()) {
    brave_wallet::ProviderErrors code =
        !success ? brave_wallet::ProviderErrors::kInternalError
                 : brave_wallet::ProviderErrors::kUserRejectedRequest;
    std::string message =
        !success ? "Internal JSON-RPC error" : "User rejected the request.";

    std::unique_ptr<base::Value> formed_response;
    formed_response = FormProviderResponse(code, message);

    v8::Local<v8::Value> result;
    result = content::V8ValueConverter::Create()->ToV8Value(
        formed_response.get(), context);

    ALLOW_UNUSED_LOCAL(resolver->Reject(context, result));
    return;
  }

  v8::Local<v8::Array> result(v8::Array::New(isolate, accounts.size()));
  for (size_t i = 0; i < accounts.size(); i++) {
    ALLOW_UNUSED_LOCAL(
        result->Set(context, i,
                    v8::String::NewFromUtf8(isolate, accounts[i].c_str())
                        .ToLocalChecked()));
  }
  ALLOW_UNUSED_LOCAL(resolver->Resolve(context, result));
}

void OnGetAllowedAccounts(v8::Global<v8::Promise::Resolver> promise_resolver,
                          v8::Isolate* isolate,
                          v8::Global<v8::Context> context_old,
                          bool success,
                          const std::vector<std::string>& accounts) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);

  if (!success) {
    brave_wallet::ProviderErrors code =
        brave_wallet::ProviderErrors::kInternalError;
    std::string message = "Internal JSON-RPC error";
    std::unique_ptr<base::Value> formed_response;
    formed_response = FormProviderResponse(code, message);

    v8::Local<v8::Value> result;
    result = content::V8ValueConverter::Create()->ToV8Value(
        formed_response.get(), context);

    ALLOW_UNUSED_LOCAL(resolver->Reject(context, result));
    return;
  }

  v8::Local<v8::Array> result(v8::Array::New(isolate, accounts.size()));
  for (size_t i = 0; i < accounts.size(); i++) {
    ALLOW_UNUSED_LOCAL(
        result->Set(context, i,
                    v8::String::NewFromUtf8(isolate, accounts[i].c_str())
                        .ToLocalChecked()));
  }
  ALLOW_UNUSED_LOCAL(resolver->Resolve(context, result));
}

}  // namespace

namespace brave_wallet {

BraveWalletJSHandler::BraveWalletJSHandler(content::RenderFrame* render_frame)
    : render_frame_(render_frame), is_connected_(false) {
  if (g_provider_script->empty()) {
    *g_provider_script =
        LoadDataResource(IDR_BRAVE_WALLET_SCRIPT_BRAVE_WALLET_SCRIPT_BUNDLE_JS);
  }
  EnsureConnected();
}

BraveWalletJSHandler::~BraveWalletJSHandler() = default;

bool BraveWalletJSHandler::EnsureConnected() {
  if (!brave_wallet_provider_.is_bound()) {
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
    v8::Isolate* isolate, v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> cosmetic_filters_obj;
  v8::Local<v8::Value> cosmetic_filters_value;
  if (!global->Get(context, gin::StringToV8(isolate, "ethereum"))
           .ToLocal(&cosmetic_filters_value) ||
      !cosmetic_filters_value->IsObject()) {
    cosmetic_filters_obj = v8::Object::New(isolate);
    global
        ->Set(context, gin::StringToSymbol(isolate, "ethereum"),
              cosmetic_filters_obj)
        .Check();
    BindFunctionsToObject(isolate, context, cosmetic_filters_obj);
  }
}

void BraveWalletJSHandler::BindFunctionsToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Object> javascript_object) {
  BindFunctionToObject(isolate, javascript_object, "request",
                       base::BindRepeating(&BraveWalletJSHandler::Request,
                                           base::Unretained(this), isolate));
  BindFunctionToObject(isolate, javascript_object, "isConnected",
                       base::BindRepeating(&BraveWalletJSHandler::IsConnected,
                                           base::Unretained(this)));
  BindFunctionToObject(isolate, javascript_object, "enable",
                       base::BindRepeating(&BraveWalletJSHandler::Enable,
                                           base::Unretained(this)));
  BindFunctionToObject(isolate, javascript_object, "sendAsync",
                       base::BindRepeating(&BraveWalletJSHandler::SendAsync,
                                           base::Unretained(this)));
  BindFunctionToObject(isolate, javascript_object, "send",
                       base::BindRepeating(&BraveWalletJSHandler::SendAsync,
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

void BraveWalletJSHandler::SendAsync(gin::Arguments* args) {
  if (!EnsureConnected())
    return;

  v8::Isolate* isolate = args->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Value> input;
  v8::Local<v8::Function> callback;
  if (!args->GetNext(&input) || !args->GetNext(&callback)) {
    args->ThrowError();
    return;
  }
  if (!input->IsObject())
    return;

  std::unique_ptr<base::Value> out(
      content::V8ValueConverter::Create()->FromV8Value(
          input, isolate->GetCurrentContext()));

  base::DictionaryValue* out_dict;
  if (!out || !out->is_dict() || !out->GetAsDictionary(&out_dict))
    return;

  // Hardcode id to 1 as it is unused
  ALLOW_UNUSED_LOCAL(out_dict->SetIntPath("id", kRequestId));
  ALLOW_UNUSED_LOCAL(out_dict->SetStringPath("jsonrpc", kRequestJsonRPC));
  std::string formed_input;
  if (!base::JSONWriter::Write(*out_dict, &formed_input))
    return;

  auto global_callback =
      std::make_unique<v8::Global<v8::Function>>(isolate, callback);

  brave_wallet_provider_->Request(
      formed_input, true,
      base::BindOnce(&BraveWalletJSHandler::OnSendAsync, base::Unretained(this),
                     std::move(global_callback)));
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
  if (!EnsureConnected() || !input->IsObject())
    return v8::Local<v8::Promise>();

  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  auto context_old(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));

  std::unique_ptr<base::Value> out(
      content::V8ValueConverter::Create()->FromV8Value(
          input, isolate->GetCurrentContext()));

  base::DictionaryValue* out_dict;
  if (!out || !out->is_dict() || !out->GetAsDictionary(&out_dict))
    return v8::Local<v8::Promise>();

  const std::string* method = out_dict->FindStringPath(kMethod);
  if (method && *method == kEthAccounts) {
    brave_wallet_provider_->GetAllowedAccounts(
        base::BindOnce(&OnGetAllowedAccounts, std::move(promise_resolver),
                       isolate, std::move(context_old)));
  } else if (method && *method == kEthRequestAccounts) {
    brave_wallet_provider_->RequestEthereumPermissions(base::BindOnce(
        &OnEthereumPermissionRequested, std::move(promise_resolver), isolate,
        std::move(context_old)));
  } else if (method && *method == kAddEthereumChainMethod) {
    const base::Value* params = out_dict->FindPath(kParams);
    if (!params)
      return v8::Local<v8::Promise>();

    mojom::EthereumChain chain;
    brave_wallet::ValueToEthereumChain(*params->GetList().begin(), &chain);

    std::vector<mojom::EthereumChainPtr> chains_ptr;
    chains_ptr.push_back(chain.Clone());

    brave_wallet_provider_->AddEthereumChain(
        std::move(chains_ptr),
        base::BindOnce(&BraveWalletJSHandler::OnRequest, base::Unretained(this),
                       std::move(promise_resolver), isolate,
                       std::move(context_old)));
  } else {
    std::string formed_input;
    // Hardcode id to 1 as it is unused
    ALLOW_UNUSED_LOCAL(out_dict->SetIntPath("id", kRequestId));
    ALLOW_UNUSED_LOCAL(out_dict->SetStringPath("jsonrpc", kRequestJsonRPC));
    if (!base::JSONWriter::Write(*out_dict, &formed_input))
      return v8::Local<v8::Promise>();
    brave_wallet_provider_->Request(
        formed_input, true,
        base::BindOnce(&BraveWalletJSHandler::OnRequest, base::Unretained(this),
                       std::move(promise_resolver), isolate,
                       std::move(context_old)));
  }

  return resolver.ToLocalChecked()->GetPromise();
}

void BraveWalletJSHandler::OnRequest(
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    v8::Global<v8::Context> context_old,
    const int http_code,
    const std::string& response,
    const base::flat_map<std::string, std::string>& headers) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  bool reject = http_code != 200;
  ProviderErrors code = ProviderErrors::kDisconnected;
  std::string message;
  std::unique_ptr<base::Value> formed_response;
  if (reject) {
    code = ProviderErrors::kUnsupportedMethod;
    message = "HTTP Status code: " + base::NumberToString(http_code);
    formed_response = FormProviderResponse(code, message);
  } else {
    formed_response = FormProviderResponse(response, false, &reject);
  }
  v8::Local<v8::Value> result;
  if (formed_response) {
    result = content::V8ValueConverter::Create()->ToV8Value(
        formed_response.get(), context);
  }

  if (reject) {
    ALLOW_UNUSED_LOCAL(resolver->Reject(context, result));
  } else {
    ALLOW_UNUSED_LOCAL(resolver->Resolve(context, result));
  }
}

v8::Local<v8::Promise> BraveWalletJSHandler::Enable() {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (!EnsureConnected() || resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }

  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  auto context_old(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  brave_wallet_provider_->RequestEthereumPermissions(base::BindOnce(
      &OnEthereumPermissionRequested, std::move(promise_resolver), isolate,
      std::move(context_old)));

  return resolver.ToLocalChecked()->GetPromise();
}

void BraveWalletJSHandler::OnSendAsync(
    std::unique_ptr<v8::Global<v8::Function>> callback,
    const int http_code,
    const std::string& response,
    const base::flat_map<std::string, std::string>& headers) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame_->GetWebFrame()->MainWorldScriptContext();
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Function> callback_local =
      v8::Local<v8::Function>::New(isolate, *callback);

  bool reject = http_code != 200;
  ProviderErrors code = ProviderErrors::kDisconnected;
  std::string message;
  std::unique_ptr<base::Value> formed_response;
  if (reject) {
    code = ProviderErrors::kUnsupportedMethod;
    message = "HTTP Status code: " + base::NumberToString(http_code);
    formed_response = FormProviderResponse(code, message);
  } else {
    formed_response = FormProviderResponse(response, true, &reject);
  }
  v8::Local<v8::Value> result;
  if (formed_response) {
    result = content::V8ValueConverter::Create()->ToV8Value(
        formed_response.get(), context);
  }

  v8::Local<v8::Value> result_null = v8::Null(isolate);
  v8::Local<v8::Value> argv[] = {reject ? result : result_null,
                                 reject ? result_null : result};

  render_frame_->GetWebFrame()->CallFunctionEvenIfScriptDisabled(
      callback_local, v8::Object::New(isolate), 2, argv);
}

void BraveWalletJSHandler::ExecuteScript(const std::string script) {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  if (web_frame->IsProvisional())
    return;

  web_frame->ExecuteScript(blink::WebString::FromUTF8(script));
}

void BraveWalletJSHandler::InjectInitScript() {
  ExecuteScript(*g_provider_script);
}

void BraveWalletJSHandler::FireEvent(const std::string& event,
                                     base::Value event_args) {
  base::Value args = base::Value(base::Value::Type::LIST);
  args.Append(event);
  args.Append(std::move(event_args));
  CallMethodOfObject(render_frame_->GetWebFrame(),
                     u"ethereum",
                     u"emit",
                     std::move(args));
}

void BraveWalletJSHandler::ConnectEvent() {
  if (!EnsureConnected())
    return;

  brave_wallet_provider_->GetChainId(base::BindOnce(
      &BraveWalletJSHandler::OnGetChainId, base::Unretained(this)));
}

void BraveWalletJSHandler::OnGetChainId(const std::string& chain_id) {
  base::DictionaryValue event_args;
  event_args.SetStringKey("chainId", chain_id);
  FireEvent(kConnectEvent, std::move(event_args));
  is_connected_ = true;
  chain_id_ = chain_id;
}

void BraveWalletJSHandler::DisconnectEvent(const std::string& message) {
  // FireEvent(kDisconnectEvent, message);
}

void BraveWalletJSHandler::ChainChangedEvent(const std::string& chain_id) {
  if (chain_id_ == chain_id)
    return;

  base::DictionaryValue event_args;
  event_args.SetStringKey("chainId", chain_id);
  FireEvent(kChainChangedEvent, std::move(event_args));
  chain_id_ = chain_id;
}

void BraveWalletJSHandler::AccountsChangedEvent(const std::string& accounts) {
  // FireEvent(kAccountsChangedEvent, accounts);
}

}  // namespace brave_wallet
