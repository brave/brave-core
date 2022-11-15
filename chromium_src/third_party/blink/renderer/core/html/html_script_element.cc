/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/html/html_script_element.h"

#define supports supports_ChromiumImpl
#include "src/third_party/blink/renderer/core/html/html_script_element.cc"
#undef supports

namespace blink {

// static
bool HTMLScriptElement::supports(ScriptState* script_state,
                                 const AtomicString& type) {
  if (type == script_type_names::kWebbundle)
    return false;

  return supports_ChromiumImpl(script_state, type);
}

}  // namespace blink
