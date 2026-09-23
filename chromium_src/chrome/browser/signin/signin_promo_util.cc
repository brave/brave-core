/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define SHOULD_SHOW_SIGNIN_PROMO_COMMON return false;
#define ShouldShowExtensionSignInPromo ShouldShowExtensionSigninPromo_UnUsed
#include <chrome/browser/signin/signin_promo_util.cc>
#undef ShouldShowExtensionSignInPromo
#undef SHOULD_SHOW_SIGNIN_PROMO_COMMON

namespace signin {

#if BUILDFLAG(ENABLE_EXTENSIONS)
bool ShouldShowExtensionSignInPromo(Profile& profile,
                                    const extensions::Extension& extension) {
  return false;
}
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

}  // namespace signin
