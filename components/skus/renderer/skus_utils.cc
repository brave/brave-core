/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/renderer/skus_utils.h"

#include <algorithm>
#include <vector>

#include "base/no_destructor.h"
#include "brave/components/skus/common/skus_utils.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_url.h"

namespace {

const std::vector<blink::WebSecurityOrigin> WebSecurityOriginList() {
  static const base::NoDestructor<std::vector<blink::WebSecurityOrigin>> list =
      [&] {
        std::vector<blink::WebSecurityOrigin> list;
        std::ranges::transform(
            skus::kSafeOrigins, std::back_inserter(list),
            [](auto& origin_string) {
              return blink::WebSecurityOrigin::Create(GURL(origin_string));
            });
        return base::NoDestructor(list);
      }();
  return *list;
}

}  // namespace

namespace skus {

bool IsSafeOrigin(const blink::WebSecurityOrigin& origin) {
  auto safe_origins = WebSecurityOriginList();
  for (const blink::WebSecurityOrigin& safe_origin : safe_origins) {
    if (safe_origin.IsSameOriginWith(origin)) {
      return true;
    }
  }
  return false;
}

}  // namespace skus
