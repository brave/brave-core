/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_PREF_NAMES_H_

namespace brave_account::prefs {

// This preference stores the entire account state as a single dictionary,
// keyed by "kind" ("logged-out" or "logged-in"). All state transitions write
// the whole dictionary in one shot, so observers see one consistent state per
// notification.
//
// First launch, no Brave Account ever used — equivalent to logged-out:
// The "account" dict in Preferences doesn't exist.
//
// Registration in progress — logged-out, with verification:
// "account": {
//   "state": {
//     "kind": "logged-out",
//     "verification": {
//       "intent": 0,
//       "token": "..."
//     }
//   }
// },
//
// Email verified, or returning user logged in — logged-in,
// optionally including service tokens:
// "account": {
//   "state": {
//     "authentication_token": "...",
//     "email": "...",
//     "kind": "logged-in",
//     "service_tokens": {
//       "email-aliases": {
//         "last_fetched": "13422130435353472",
//         "service_token": "..."
//       }
//     }
//   }
// },
//
// Password change in progress — logged-in, with verification:
// "account": {
//   "state": {
//     "authentication_token": "...",
//     "email": "...",
//     "kind": "logged-in"
//     "verification": {
//       "intent": 0,
//       "token": "..."
//     }
//   }
// },
//
// Logged out — logged-out:
// "account": {
//   "state": {
//     "kind": "logged-out"
//   }
// },
//
// Password reset in progress — logged-out, with verification:
// "account": {
//   "state": {
//     "kind": "logged-out",
//     "verification": {
//       "intent": 1,
//       "token": "..."
//     }
//   }
// },
inline constexpr char kBraveAccountState[] = "brave.account.state";

namespace keys {

// Top-level dictionary keys used within kBraveAccountState.
inline constexpr char kAuthenticationToken[] = "authentication_token";
inline constexpr char kEmail[] = "email";
inline constexpr char kKind[] = "kind";
inline constexpr char kServiceTokens[] = "service_tokens";
inline constexpr char kVerification[] = "verification";

// Dictionary keys used within the "verification" sub-dict of
// kBraveAccountState.
inline constexpr char kVerificationIntent[] = "intent";
inline constexpr char kVerificationToken[] = "token";

// Dictionary keys used within entries of the "service_tokens" sub-dict of
// kBraveAccountState.
inline constexpr char kLastFetched[] = "last_fetched";
inline constexpr char kServiceToken[] = "service_token";

}  // namespace keys

namespace state_kinds {

// Values for the kKind field within kBraveAccountState.
inline constexpr char kLoggedIn[] = "logged-in";
inline constexpr char kLoggedOut[] = "logged-out";

}  // namespace state_kinds

}  // namespace brave_account::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_PREF_NAMES_H_
