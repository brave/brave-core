/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/dom/node.cc"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/string_view.cc"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

AtomicString PossiblyModifyAttrParam(const Element& elm,
    const QualifiedName& name, const AtomicString& new_value) {
  // Check to see if the following conditions are true, to work around
  // the google tag manager (and related) block-the-screen stuff.
  //   1) The class being modified
  //   2) The element being modified is the <html> element
  //   3) The new class value includes "async-hide"
  // https://github.com/brave/brave-browser/issues/4402
  const StringView async_hide_class_token("async-hide");
  const bool should_strip_async_hide_class = (
    name == kClassAttr &&
    elm.tagName().LowerASCII() == "html" &&
    new_value.Find(async_hide_class_token) != kNotFound);

  if (!should_strip_async_hide_class) {
    return new_value;
  }

  String prev_attr_value = new_value.GetString();
  AtomicString modified_value(prev_attr_value.Replace(
    async_hide_class_token, StringView("")));

  return modified_value;
}

}  // namespace blink

