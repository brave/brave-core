/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/renderer/skus_utils.h"

#include <vector>

#include "base/no_destructor.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_url.h"
#include "url/gurl.h"

namespace skus {

bool IsSafeOrigin(const blink::WebSecurityOrigin& origin) {
  // NOTE: please open a security review when appending to this list.
  static base::NoDestructor<std::vector<blink::WebSecurityOrigin>> safe_origins{
      {{blink::WebSecurityOrigin::Create(GURL("https://account.brave.com"))},
       {blink::WebSecurityOrigin::Create(
           GURL("https://account.bravesoftware.com"))},
       {blink::WebSecurityOrigin::Create(
           GURL("https://account.brave.software"))}}};

  for (const blink::WebSecurityOrigin& safe_origin : *safe_origins) {
    if (safe_origin.IsSameOriginWith(origin)) {
      return true;
    }
  }
  return false;
}

}  // namespace skus
