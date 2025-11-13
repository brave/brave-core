/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_PREF_NAMES_H_

namespace brave_account::prefs {

// This preference will store (`OSCrypt`-encrypted) the `verificationToken`
// returned by the POST /v2/accounts/password/init endpoint.
// It is then used for:
// - calling POST /v2/accounts/password/finalize
//   to complete the registration flow
// - polling POST /v2/verify/result
//   to exchange the `verificationToken` for the `authToken`
//   after the user has verified their email
inline constexpr char kBraveAccountVerificationToken[] =
    "brave.account.verification_token";

// This preference will store (`OSCrypt`-encrypted) the (JWT) `authToken`
// returned by the POST /v2/verify/result endpoint.
// It is then used for:
// - authenticating subsequent Brave Account requests
// - creating new service tokens
inline constexpr char kBraveAccountAuthenticationToken[] =
    "brave.account.authentication_token";

}  // namespace brave_account::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_PREF_NAMES_H_
