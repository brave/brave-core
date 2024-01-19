/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/mac/keystone_glue.h"

#define GetBrand GetBrand_ChromiumImpl
#include "src/chrome/browser/google/google_brand_mac.mm"
#undef GetBrand

namespace google_brand {

bool GetBrand(std::string* brand) {
  if (g_brand_for_testing) {
    return GetBrand_ChromiumImpl(brand);
  }
  brand->assign(keystone_glue::BrandCode());
  return true;
}

}  // namespace google_brand
