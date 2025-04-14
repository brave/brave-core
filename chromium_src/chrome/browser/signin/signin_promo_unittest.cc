/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/signin/signin_promo_unittest.cc"

namespace signin {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)

class ShowSigninPromoTestWithFeatureFlagsIsDisabled
    : public ShowSigninPromoTestWithFeatureFlags {};

TEST_F(ShowSigninPromoTestWithFeatureFlagsIsDisabled, ShowPromoWithNoAccount) {
  EXPECT_FALSE(ShouldShowPasswordSignInPromo(*profile()));
}

TEST_F(ShowSigninPromoTestWithFeatureFlagsIsDisabled,
       ShowPromoWithWebSignedInAccount) {
  MakeAccountAvailable(identity_manager(), "test@email.com");
  EXPECT_FALSE(ShouldShowPasswordSignInPromo(*profile()));
}

TEST_F(ShowSigninPromoTestWithFeatureFlagsIsDisabled,
       ShowPromoWithSignInPendingAccount) {
  AccountInfo info = MakePrimaryAccountAvailable(
      identity_manager(), "test@email.com", ConsentLevel::kSignin);
  signin::SetInvalidRefreshTokenForPrimaryAccount(identity_manager());
  EXPECT_FALSE(ShouldShowPasswordSignInPromo(*profile()));
}

TEST_F(ShowSigninPromoTestWithFeatureFlagsIsDisabled, DoNotShowAddressPromo) {
  ASSERT_FALSE(ShouldShowAddressSignInPromo(*profile(), CreateAddress()));
}

TEST_F(ShowSigninPromoTestWithFeatureFlagsIsDisabled, DoNotShowBookmarkPromo) {
  ASSERT_FALSE(ShouldShowBookmarkSignInPromo(*profile()));
}

#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

}  // namespace signin
