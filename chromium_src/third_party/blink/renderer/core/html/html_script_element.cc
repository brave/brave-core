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
bool HTMLScriptElement::supports(const AtomicString& type) {
  if (type == script_type_names::kWebbundle)
    return false;

  // There used to be a kSpeculationRulesPrefetchProxy feature flag to disable
  // speculative prefetching in the upstream function, but with its removal, it
  // was necessary to move the check here.
  if (type == script_type_names::kSpeculationrules) {
    return false;
  }

  return supports_ChromiumImpl(type);
}

}  // namespace blink
