/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/signin/signin_promo_unittest.cc"

namespace signin {

#if BUILDFLAG(ENABLE_DICE_SUPPORT)

// Creating a derived class, so disabling
// `ShowSigninPromoTestExplicitBrowserSignin` in filter files won't affect these
// disable tests.
class ShowSigninPromoTestExplicitBrowserSigninIsDisabled
    : public ShowSigninPromoTestWithFeatureFlags {};

TEST_F(ShowSigninPromoTestExplicitBrowserSigninIsDisabled,
       ShowPromoWithNoAccount) {
  EXPECT_FALSE(ShouldShowPasswordSignInPromo(*profile()));
}

TEST_F(ShowSigninPromoTestExplicitBrowserSigninIsDisabled,
       ShowPromoWithWebSignedInAccount) {
  MakeAccountAvailable(identity_manager(), "test@email.com");
  EXPECT_FALSE(ShouldShowPasswordSignInPromo(*profile()));
}

TEST_F(ShowSigninPromoTestExplicitBrowserSigninIsDisabled,
       ShowPromoWithSignInPendingAccount) {
  AccountInfo info = MakePrimaryAccountAvailable(
      identity_manager(), "test@email.com", ConsentLevel::kSignin);
  signin::SetInvalidRefreshTokenForPrimaryAccount(identity_manager());
  EXPECT_FALSE(ShouldShowPasswordSignInPromo(*profile()));
}

TEST_F(ShowSigninPromoTestExplicitBrowserSigninIsDisabled,
       DoNotShowAddressPromo) {
  ASSERT_FALSE(ShouldShowAddressSignInPromo(*profile(), CreateAddress()));
}

TEST_F(ShowSigninPromoTestExplicitBrowserSigninIsDisabled,
       DoNotShowBookmarkPromo) {
  ASSERT_FALSE(ShouldShowBookmarkSignInPromo(*profile()));
}

TEST_F(ShowSigninPromoTestExplicitBrowserSigninIsDisabled,
       ShowExtensionsPromoWithNoAccount) {
  EXPECT_FALSE(ShouldShowExtensionSignInPromo(*profile(), *CreateExtension()));
}

#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

}  // namespace signin
