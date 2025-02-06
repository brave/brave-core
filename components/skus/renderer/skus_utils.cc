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
constexpr auto kSafeOrigins = std::to_array<std::string_view>({
    "https://account.brave.com",
    "https://account.bravesoftware.com",
    "https://account.brave.software",
});

std::vector<blink::WebSecurityOrigin> WebSecurityOriginList() {
  std::vector<blink::WebSecurityOrigin> list(kSafeOrigins.size());
  std::transform(kSafeOrigins.begin(), kSafeOrigins.end(), list.begin(),
                 [](auto& origin_string) {
                   return blink::WebSecurityOrigin::Create(GURL(origin_string));
                 });
  return list;
}

std::vector<url::Origin> OriginList() {
  std::vector<url::Origin> list(kSafeOrigins.size());
  std::transform(kSafeOrigins.begin(), kSafeOrigins.end(), list.begin(),
                 [](auto& origin_string) {
                   return url::Origin::Create(GURL(origin_string));
                 });
  return list;
}

}  // namespace

namespace skus {
bool IsSafeOrigin(const blink::WebSecurityOrigin& origin) {
  static base::NoDestructor<std::vector<blink::WebSecurityOrigin>>
      safe_origins = base::NoDestructor(WebSecurityOriginList());
  for (const blink::WebSecurityOrigin& safe_origin : *safe_origins) {
    if (safe_origin.IsSameOriginWith(origin)) {
      return true;
    }
  }
  return false;
}

bool IsSafeOrigin(const GURL& origin) {
  static base::NoDestructor<std::vector<url::Origin>> safe_origins =
      base::NoDestructor(OriginList());
  for (const url::Origin& safe_origin : *safe_origins) {
    if (safe_origin.IsSameOriginWith(origin)) {
      return true;
    }
  }
  return false;
}

}  // namespace skus
