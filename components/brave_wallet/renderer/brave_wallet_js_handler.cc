/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_js_handler.h"

#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
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

void BraveWalletJSHandler::OnEthereumPermissionRequested(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    bool success,
    const std::vector<std::string>& accounts) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  std::unique_ptr<base::Value> formed_response;
  if (!success || accounts.empty()) {
    brave_wallet::ProviderErrors code =
        !success ? brave_wallet::ProviderErrors::kInternalError
                 : brave_wallet::ProviderErrors::kUserRejectedRequest;
    std::string message =
        !success ? "Internal JSON-RPC error" : "User rejected the request.";

    formed_response = GetProviderErrorDictionary(code, message);
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

void BraveWalletJSHandler::OnGetAllowedAccounts(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    bool success,
    const std::vector<std::string>& accounts) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  std::unique_ptr<base::Value> formed_response;
  if (!success) {
    brave_wallet::ProviderErrors code =
        brave_wallet::ProviderErrors::kInternalError;
    std::string message = "Internal JSON-RPC error";
    formed_response = GetProviderErrorDictionary(code, message);
  } else {
    formed_response = base::Value::ToUniquePtrValue(base::ListValue());
    for (size_t i = 0; i < accounts.size(); i++) {
      formed_response->Append(base::Value(accounts[i]));
    }
  }

  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response), success);
}

void BraveWalletJSHandler::OnAddEthereumChain(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    bool success,
    int provider_error,
    const std::string& error_message) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  std::unique_ptr<base::Value> formed_response;
  if (success) {
    formed_response = base::Value::ToUniquePtrValue(base::Value());
  } else {
    formed_response = GetProviderErrorDictionary(
        static_cast<brave_wallet::ProviderErrors>(provider_error),
        error_message);
  }
  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response), success);
}

void BraveWalletJSHandler::OnAddAndApproveTransaction(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    bool success,
    const std::string& tx_hash,
    const std::string& error_message) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  std::unique_ptr<base::Value> formed_response;
  if (success) {
    formed_response = base::Value::ToUniquePtrValue(base::Value(tx_hash));
  } else {
    formed_response = GetProviderErrorDictionary(
        brave_wallet::ProviderErrors::kInvalidParams, error_message);
  }

  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response), success);
}

void BraveWalletJSHandler::OnSignMessage(
    base::Value id,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response,
    const std::string& signature,
    int error,
    const std::string& error_message) {
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  std::unique_ptr<base::Value> formed_response;
  if (!error) {
    formed_response = base::Value::ToUniquePtrValue(base::Value(signature));
  } else {
    formed_response = GetProviderErrorDictionary(
        static_cast<brave_wallet::ProviderErrors>(error), error_message);
  }

  SendResponse(std::move(id), std::move(global_context),
               std::move(global_callback), std::move(promise_resolver), isolate,
               force_json_response, std::move(formed_response), !error);
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
    v8::Local<v8::Function> local_callback =
        v8::Local<v8::Function>::New(isolate, *global_callback);
    v8::Local<v8::Value> result_null = v8::Null(isolate);
    v8::Local<v8::Value> argv[] = {success ? result_null : result,
                                   success ? result : result_null};
    render_frame_->GetWebFrame()->CallFunctionEvenIfScriptDisabled(
        local_callback, v8::Object::New(isolate), 2, argv);
    return;
  }

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  if (success) {
    ALLOW_UNUSED_LOCAL(resolver->Resolve(context, result));
  } else {
    ALLOW_UNUSED_LOCAL(resolver->Reject(context, result));
  }
}

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
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> ethereum_obj;
  v8::Local<v8::Value> ethereum_value;
  if (!global->Get(context, gin::StringToV8(isolate, "ethereum"))
           .ToLocal(&ethereum_value) ||
      !ethereum_value->IsObject()) {
    ethereum_obj = v8::Object::New(isolate);
    global->Set(context, gin::StringToSymbol(isolate, "ethereum"), ethereum_obj)
        .Check();
    BindFunctionsToObject(isolate, context, ethereum_obj);
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
  BindFunctionToObject(
      isolate, javascript_object, "send",
      base::BindRepeating(&BraveWalletJSHandler::Send, base::Unretained(this)));
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

bool BraveWalletJSHandler::CommonRequestOrSendAsync(
    std::unique_ptr<base::Value> input_value,
    v8::Global<v8::Context> global_context,
    std::unique_ptr<v8::Global<v8::Function>> global_callback,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool force_json_response) {
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
    brave_wallet_provider_->GetAllowedAccounts(base::BindOnce(
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
        base::BindOnce(&BraveWalletJSHandler::OnAddEthereumChain,
                       weak_ptr_factory_.GetWeakPtr(), std::move(id),
                       std::move(global_context), std::move(global_callback),
                       std::move(promise_resolver), isolate,
                       force_json_response));
  } else if (method == kEthSendTransaction) {
    std::string from;
    auto tx_data =
        ParseEthSendTransactionParams(normalized_json_request, &from);
    if (tx_data && !tx_data->gas_price.empty()) {
      brave_wallet_provider_->AddAndApproveTransaction(
          std::move(tx_data), from,
          base::BindOnce(&BraveWalletJSHandler::OnAddAndApproveTransaction,
                         weak_ptr_factory_.GetWeakPtr(), std::move(id),
                         std::move(global_context), std::move(global_callback),
                         std::move(promise_resolver), isolate,
                         force_json_response));
    } else {
      from.clear();
      mojom::TxData1559Ptr tx_data_1559 =
          ParseEthSendTransaction1559Params(normalized_json_request, &from);
      if (!tx_data_1559)
        tx_data_1559 = mojom::TxData1559::New();
      brave_wallet_provider_->AddAndApprove1559Transaction(
          std::move(tx_data_1559), from,
          base::BindOnce(&BraveWalletJSHandler::OnAddAndApproveTransaction,
                         weak_ptr_factory_.GetWeakPtr(), std::move(id),
                         std::move(global_context), std::move(global_callback),
                         std::move(promise_resolver), isolate,
                         force_json_response));
    }
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
        base::BindOnce(&BraveWalletJSHandler::OnSignMessage,
                       weak_ptr_factory_.GetWeakPtr(), std::move(id),
                       std::move(global_context), std::move(global_callback),
                       std::move(promise_resolver), isolate,
                       force_json_response));
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
// Promise<JsonRpcResponse>; method and parameters specified instead of inside a
// JSON-RPC payload
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
  if (!CommonRequestOrSendAsync(
          std::move(input_value), std::move(global_context),
          std::move(global_callback), v8::Global<v8::Promise::Resolver>(),
          isolate, true)) {
    base::Value id;
    std::string input_json;
    if (base::JSONWriter::Write(*input_value, &input_json)) {
      std::string method;
      ALLOW_UNUSED_LOCAL(
          GetEthJsonRequestInfo(input_json, &id, nullptr, nullptr));
    }

    auto global_context(
        v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
    auto global_callback =
        std::make_unique<v8::Global<v8::Function>>(isolate, callback);
    ProviderErrors code = ProviderErrors::kUnsupportedMethod;
    std::string message = "Generic processing error";
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

  if (!CommonRequestOrSendAsync(
          std::move(input_value), std::move(global_context), nullptr,
          std::move(promise_resolver), isolate, force_json_response)) {
    ProviderErrors code = ProviderErrors::kUnsupportedMethod;
    std::string message = "Generic processing error";
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
  v8::HandleScope handle_scope(isolate);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
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
  if (!EnsureConnected() || resolver.IsEmpty()) {
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

void BraveWalletJSHandler::AccountsChangedEvent(
    const std::vector<std::string>& accounts) {
  base::ListValue event_args;
  for (const std::string& account : accounts) {
    event_args.Append(base::Value(account));
  }
  FireEvent(kAccountsChangedEvent, std::move(event_args));
}

}  // namespace brave_wallet
