/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/exported/web_document.cc"

#include "third_party/blink/renderer/core/permissions_policy/dom_feature_policy.h"

namespace blink {

bool WebDocument::IsDOMFeaturePolicyEnabled(v8::Isolate* isolate,
                                            v8::Local<v8::Context> context,
                                            const WebString& feature) {
  blink::ScriptState* script_state = blink::ScriptState::From(isolate, context);
  Document* document = Unwrap<Document>();
  return document->featurePolicy()->allowsFeature(script_state, feature);
}

}  // namespace blink
