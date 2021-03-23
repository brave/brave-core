/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/renderer/brave_search_js_handler.h"

#include <utility>

#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace brave_search {

BraveSearchJSHandler::BraveSearchJSHandler(content::RenderFrame* render_frame)
    : render_frame_(render_frame) {}

BraveSearchJSHandler::~BraveSearchJSHandler() = default;

bool BraveSearchJSHandler::EnsureConnected() {
  if (!brave_search_fallback_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        brave_search_fallback_.BindNewPipeAndPassReceiver());
  }

  return brave_search_fallback_.is_bound();
}

void BraveSearchJSHandler::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  CreateAFallbackObject(isolate, context);
}

void BraveSearchJSHandler::CreateAFallbackObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Value> chrome_value;
  if (global->Get(context, gin::StringToV8(isolate, "chrome"))
          .ToLocal(&chrome_value) &&
      chrome_value->IsObject()) {
    BindFunctionsToObject(isolate, context,
                          chrome_value->ToObject(context).ToLocalChecked());
  }
}

void BraveSearchJSHandler::BindFunctionsToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Object> javascript_object) {
  BindFunctionToObject(
      isolate, javascript_object, "fetchBackupResults",
      base::BindRepeating(&BraveSearchJSHandler::FetchBackupResults,
                          base::Unretained(this), isolate));
}

template <typename Sig>
void BraveSearchJSHandler::BindFunctionToObject(
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

v8::Local<v8::Promise> BraveSearchJSHandler::FetchBackupResults(
    v8::Isolate* isolate,
    const std::string& query_string,
    const std::string& lang,
    const std::string& country,
    const std::string& geo) {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();

  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (!resolver.IsEmpty()) {
    auto promise_resolver =
        std::make_unique<v8::Global<v8::Promise::Resolver>>();
    promise_resolver->Reset(isolate, resolver.ToLocalChecked());
    auto context_old = std::make_unique<v8::Global<v8::Context>>(
        isolate, isolate->GetCurrentContext());
    brave_search_fallback_->FetchBackupResults(
        query_string, lang, country, geo,
        base::BindOnce(&BraveSearchJSHandler::OnFetchBackupResults,
                       base::Unretained(this), std::move(promise_resolver),
                       isolate, std::move(context_old)));

    return resolver.ToLocalChecked()->GetPromise();
  }

  return v8::Local<v8::Promise>();
}

void BraveSearchJSHandler::OnFetchBackupResults(
    std::unique_ptr<v8::Global<v8::Promise::Resolver>> promise_resolver,
    v8::Isolate* isolate,
    std::unique_ptr<v8::Global<v8::Context>> context_old,
    const std::string& response) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old->Get(isolate);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver->Get(isolate);
  v8::Local<v8::String> result;
  result = v8::String::NewFromUtf8(isolate, response.c_str()).ToLocalChecked();

  ALLOW_UNUSED_LOCAL(resolver->Resolve(context, result));
}

}  // namespace brave_search
