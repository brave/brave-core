/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define ShouldShowPromo ShouldShowPromo_ChromiumImpl
#define ShouldShowSyncPromo ShouldShowSyncPromo_ChromiumImpl
#include "src/chrome/browser/signin/signin_promo_util.cc"
#undef ShouldShowSyncPromo
#undef ShouldShowPromo

namespace signin {

bool ShouldShowPromo(Profile& profile, ConsentLevel promo_type) {
  return false;
}

#if !BUILDFLAG(IS_ANDROID)
bool ShouldShowSyncPromo(Profile& profile) {
  return false;
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace signin
