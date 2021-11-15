/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/script/detect_javascript_frameworks.h"

#define DetectJavascriptFrameworksOnLoad DetectJavascriptFrameworksOnLoad_Unused
#include "../../../../../../../third_party/blink/renderer/core/script/detect_javascript_frameworks.cc"
#undef DetectJavascriptFrameworksOnLoad

namespace blink {

namespace {

// NOLINTNEXTLINE(runtime/references)
void CheckForGatsby(Document& document, v8::Local<v8::Context> context) {
  if (IsFrameworkIDUsed(document, kGatsbyId)) {
    document.Loader()->DidObserveLoadingBehavior(
        kLoadingBehaviorGatsbyFrameworkUsed);
  }
}

// NOLINTNEXTLINE(runtime/references)
void CheckForNextJS(Document& document, v8::Local<v8::Context> context) {
  if (IsFrameworkIDUsed(document, kNextjsId) &&
      IsFrameworkVariableUsed(context, kNextjsData)) {
    document.Loader()->DidObserveLoadingBehavior(
        LoadingBehaviorFlag::kLoadingBehaviorNextJSFrameworkUsed);
  }
}

// NOLINTNEXTLINE(runtime/references)
void CheckForNuxtJS(Document& document, v8::Local<v8::Context> context) {
  if (IsFrameworkVariableUsed(context, kNuxtjsData)) {
    document.Loader()->DidObserveLoadingBehavior(
        kLoadingBehaviorNuxtJSFrameworkUsed);
  }
}

// NOLINTNEXTLINE(runtime/references)
void CheckForSapper(Document& document, v8::Local<v8::Context> context) {
  if (IsFrameworkVariableUsed(context, kSapperData)) {
    document.Loader()->DidObserveLoadingBehavior(
        kLoadingBehaviorSapperFrameworkUsed);
  }
}

// NOLINTNEXTLINE(runtime/references)
void CheckForVuePress(Document& document, v8::Local<v8::Context> context) {
  if (IsFrameworkVariableUsed(context, kVuepressData)) {
    document.Loader()->DidObserveLoadingBehavior(
        kLoadingBehaviorVuePressFrameworkUsed);
  }
}

}  // namespace

// NOLINTNEXTLINE(runtime/references)
void DetectJavascriptFrameworksOnLoad(Document& document) {
  // Only detect Javascript frameworks on the main frame and if URL and BaseURL
  // is HTTP. Note: Without these checks, ToScriptStateForMainWorld will
  // initialize WindowProxy and trigger a second DidClearWindowObject() earlier
  // than expected for Android WebView. The Gin Java Bridge has a race condition
  // that relies on a second DidClearWindowObject() firing immediately before
  // executing JavaScript. See the document that explains this in more detail:
  // https://docs.google.com/document/d/1R5170is5vY425OO2Ru-HJBEraEKu0HjQEakcYldcSzM/edit?usp=sharing
  if (!document.GetFrame() || !document.GetFrame()->IsMainFrame() ||
      !document.Url().ProtocolIsInHTTPFamily() ||
      !document.BaseURL().ProtocolIsInHTTPFamily()) {
    return;
  }

  ScriptState* script_state = ToScriptStateForMainWorld(document.GetFrame());

  if (!script_state || !script_state->ContextIsValid()) {
    return;
  }

  ScriptState::Scope scope(script_state);
  v8::Local<v8::Context> context = script_state->GetContext();

  // Do not report all JavaScript frameworks from Brave, just the simpler ones.
  CheckForGatsby(document, context);
  CheckForNextJS(document, context);
  CheckForNuxtJS(document, context);
  CheckForSapper(document, context);
  CheckForVuePress(document, context);
}

}  // namespace blink
