/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/renderer/brave_search_default_js_handler.h"

#include <tuple>
#include <utility>

#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace brave_search {

BraveSearchDefaultJSHandler::BraveSearchDefaultJSHandler(
    content::RenderFrame* render_frame,
    bool can_always_set_default)
    : render_frame_(render_frame),
      can_always_set_default_(can_always_set_default) {}

BraveSearchDefaultJSHandler::~BraveSearchDefaultJSHandler() = default;

bool BraveSearchDefaultJSHandler::EnsureConnected() {
  if (!brave_search_default_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker().GetInterface(
        brave_search_default_.BindNewPipeAndPassReceiver());
    if (can_always_set_default_)
      brave_search_default_->SetCanAlwaysSetDefault();
  }

  return brave_search_default_.is_bound();
}

void BraveSearchDefaultJSHandler::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  CHECK(render_frame_);
  v8::Isolate* isolate =
      render_frame_->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  BindFunctionsToObject(isolate, context);
}

void BraveSearchDefaultJSHandler::ResetRemote(
    content::RenderFrame* render_frame) {
  render_frame_ = render_frame;
  brave_search_default_.reset();
  EnsureConnected();
}

void BraveSearchDefaultJSHandler::BindFunctionsToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> brave_obj;
  v8::Local<v8::Value> brave_value;
  if (!global->Get(context, gin::StringToV8(isolate, "brave"))
           .ToLocal(&brave_value) ||
      !brave_value->IsObject()) {
    brave_obj = v8::Object::New(isolate);
    global->Set(context, gin::StringToSymbol(isolate, "brave"), brave_obj)
        .Check();
  } else {
    brave_obj = brave_value->ToObject(context).ToLocalChecked();
  }
  BindFunctionToObject(
      isolate, brave_obj, "getCanSetDefaultSearchProvider",
      base::BindRepeating(
          &BraveSearchDefaultJSHandler::GetCanSetDefaultSearchProvider,
          base::Unretained(this), isolate));
  BindFunctionToObject(
      isolate, brave_obj, "setIsDefaultSearchProvider",
      base::BindRepeating(
          &BraveSearchDefaultJSHandler::SetIsDefaultSearchProvider,
          base::Unretained(this), isolate));
}

template <typename Sig>
void BraveSearchDefaultJSHandler::BindFunctionToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> javascript_object,
    const std::string& name,
    const base::RepeatingCallback<Sig>& callback) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  javascript_object
      ->Set(context, gin::StringToSymbol(isolate, name),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

v8::Local<v8::Promise>
BraveSearchDefaultJSHandler::GetCanSetDefaultSearchProvider(
    v8::Isolate* isolate) {
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
    brave_search_default_->GetCanSetDefaultSearchProvider(base::BindOnce(
        &BraveSearchDefaultJSHandler::OnCanSetDefaultSearchProvider,
        base::Unretained(this), std::move(promise_resolver), isolate,
        std::move(context_old)));

    return resolver.ToLocalChecked()->GetPromise();
  }

  return v8::Local<v8::Promise>();
}

void BraveSearchDefaultJSHandler::OnCanSetDefaultSearchProvider(
    std::unique_ptr<v8::Global<v8::Promise::Resolver>> promise_resolver,
    v8::Isolate* isolate,
    std::unique_ptr<v8::Global<v8::Context>> context_old,
    const bool response) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old->Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver->Get(isolate);
  v8::Local<v8::Boolean> result;
  result = v8::Boolean::New(isolate, response);

  std::ignore = resolver->Resolve(context, result);
}

void BraveSearchDefaultJSHandler::SetIsDefaultSearchProvider(
    v8::Isolate* isolate) {
  if (!EnsureConnected())
    return;
  auto* web_frame = render_frame_->GetWebFrame();
  // Prevent site from calling this in response to a DOM event or Timer.
  if (web_frame->HasTransientUserActivation()) {
    brave_search_default_->SetIsDefaultSearchProvider();
  } else {
    blink::WebString message =
        "setIsDefaultSearchProvider: "
        "API can only be initiated by a user gesture.";
    web_frame->AddMessageToConsole(blink::WebConsoleMessage(
        blink::mojom::ConsoleMessageLevel::kWarning, message));
  }
}

}  // namespace brave_search
