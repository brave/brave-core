/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/common/skus_utils.h"

#include <algorithm>
#include <vector>

#include "base/no_destructor.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

const std::vector<url::Origin>& OriginList() {
  static const base::NoDestructor<std::vector<url::Origin>> list([] {
    std::vector<url::Origin> list;
    std::ranges::transform(skus::kSafeOrigins, std::back_inserter(list),
                           [](auto& origin_string) {
                             return url::Origin::Create(GURL(origin_string));
                           });
    return list;
  }());
  return *list;
}

}  // namespace

namespace skus {

bool IsSafeOrigin(const GURL& origin) {
  auto safe_origins = OriginList();
  for (const url::Origin& safe_origin : safe_origins) {
    if (safe_origin.IsSameOriginWith(origin)) {
      return true;
    }
  }
  return false;
}

}  // namespace skus
