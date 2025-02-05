/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/renderer/skus_utils.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/no_destructor.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_url.h"
#include "url/gurl.h"

namespace {
// NOTE: please open a security review when appending to this list.
std::vector<GURL> safe_origins_gurl{GURL("https://account.brave.com"),
                                    GURL("https://account.bravesoftware.com"),
                                    GURL("https://account.brave.software")};

base::NoDestructor<std::vector<blink::WebSecurityOrigin>>
WebSecurityOriginList() {
  std::vector<blink::WebSecurityOrigin> list(safe_origins_gurl.size());
  std::transform(safe_origins_gurl.begin(), safe_origins_gurl.end(),
                 list.begin(), [](auto& origin_gurl) {
                   return blink::WebSecurityOrigin::Create(origin_gurl);
                 });
  return base::NoDestructor(list);
}

}  // namespace

namespace skus {
bool IsSafeOrigin(const blink::WebSecurityOrigin& origin) {
  static base::NoDestructor<std::vector<blink::WebSecurityOrigin>>
      safe_origins = WebSecurityOriginList();
  for (const blink::WebSecurityOrigin& safe_origin : *safe_origins) {
    if (safe_origin.IsSameOriginWith(origin)) {
      return true;
    }
  }
  return false;
}

bool IsSafeOrigin(const GURL& origin) {
  for (const GURL& safe_origin_gurl : safe_origins_gurl) {
    auto safe_origin = url::Origin::Create(safe_origin_gurl);
    if (safe_origin.IsSameOriginWith(origin)) {
      return true;
    }
  }
  return false;
}

}  // namespace skus
