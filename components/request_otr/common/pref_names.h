// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_REQUEST_OTR_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_REQUEST_OTR_COMMON_PREF_NAMES_H_

namespace request_otr {

// Action for Request Off-The-Record feature
inline constexpr char kRequestOTRActionOption[] =
    "brave.request_otr.request_otr_action_option";
inline constexpr char kInterstitialShownStorage[] =
    "brave.request_otr.p3a.interstitial_shown_storage";
inline constexpr char kInterstitialDurationStorage[] =
    "brave.request_otr.p3a.interstitial_duration_storage";
inline constexpr char kSessionCountStorage[] =
    "brave.request_otr.p3a.session_count_storage";

}  // namespace request_otr

#endif  // BRAVE_COMPONENTS_REQUEST_OTR_COMMON_PREF_NAMES_H_
