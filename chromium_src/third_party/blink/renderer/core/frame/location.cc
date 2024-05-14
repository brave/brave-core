/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/location.h"

#define ancestorOrigins ancestorOrigins_ChromiumImpl
#include "src/third_party/blink/renderer/core/frame/location.cc"
#undef ancestorOrigins

namespace blink {

DOMStringList* Location::ancestorOrigins() const {
  auto* raw_origins = ancestorOrigins_ChromiumImpl();
  if (!IsAttached() || !raw_origins || raw_origins->IsEmpty()) {
    return raw_origins;
  }

  auto* filtered_origins = MakeGarbageCollected<DOMStringList>();
  const auto* innermost_origin =
      DomWindow()->GetFrame()->GetSecurityContext()->GetSecurityOrigin();

  auto is_onion_service = [](const SecurityOrigin* origin) {
    return origin->Host().EndsWith(".onion", kTextCaseASCIIInsensitive);
  };

  auto is_chrome_untrusted = [](const SecurityOrigin* origin) {
    return origin->Protocol() == "chrome-untrusted";
  };

  for (uint32_t i = 0; i < raw_origins->length(); ++i) {
    const String raw_origin = raw_origins->item(i);
    const scoped_refptr<SecurityOrigin> origin =
        SecurityOrigin::CreateFromString(raw_origin);
    if (is_chrome_untrusted(origin.get())) {
      break;
    }
    if (is_onion_service(origin.get()) &&
        !origin->IsSameOriginWith(innermost_origin)) {
      filtered_origins->Append("\"null\"");
    } else {
      filtered_origins->Append(raw_origin);
    }
  }

  return filtered_origins;
}

}  // namespace blink
