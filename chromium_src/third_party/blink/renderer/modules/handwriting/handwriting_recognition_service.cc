/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/handwriting/handwriting_recognition_service.h"

#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"

namespace blink {

namespace {

constexpr char kHandwritingDisabledError[] =
    "Feature \"Handwriting Recognition\" is disabled.";

}  // namespace

HandwritingRecognitionService::HandwritingRecognitionService(
    Navigator& navigator)
    : navigator_(navigator), remote_service_(navigator.GetExecutionContext()) {}

// IDL Interface:
// static
ScriptPromise<HandwritingRecognizer>
HandwritingRecognitionService::createHandwritingRecognizer(
    ScriptState*,
    Navigator&,
    const HandwritingModelConstraint*,
    ExceptionState& exception_state) {
  exception_state.ThrowDOMException(DOMExceptionCode::kNotSupportedError,
                                    kHandwritingDisabledError);
  return ScriptPromise<HandwritingRecognizer>();
}

// static
ScriptPromise<IDLNullable<HandwritingRecognizerQueryResult>>
HandwritingRecognitionService::queryHandwritingRecognizer(
    ScriptState*,
    Navigator&,
    const HandwritingModelConstraint*,
    ExceptionState& exception_state) {
  exception_state.ThrowDOMException(DOMExceptionCode::kNotSupportedError,
                                    kHandwritingDisabledError);
  return ScriptPromise<IDLNullable<HandwritingRecognizerQueryResult>>();
}

void HandwritingRecognitionService::Trace(Visitor* visitor) const {
  visitor->Trace(remote_service_);
  visitor->Trace(navigator_);
}

}  // namespace blink
