/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_PREF_NAMES_H_

#include "base/component_export.h"
#include "build/build_config.h"

class PrefRegistrySimple;

namespace brave_origin {

// Base key for all profile-scoped BraveOrigin policy preferences in local state
inline constexpr char kBraveOriginPolicies[] = "brave.brave_origin.policies";

namespace prefs {

#if BUILDFLAG(IS_ANDROID)
inline constexpr char kBraveOriginSubscriptionActiveAndroid[] =
    "brave.origin.subscription_active_android";
inline constexpr char kBraveOriginPurchaseTokenAndroid[] =
    "brave.origin.purchase_token_android";
inline constexpr char kBraveOriginPackageNameAndroid[] =
    "brave.origin.package_name_android";
inline constexpr char kBraveOriginProductIdAndroid[] =
    "brave.origin.product_id_android";
inline constexpr char kBraveOriginOrderIdAndroid[] =
    "brave.origin.order_id_android";
inline constexpr char kBraveOriginSubscriptionLinkStatusAndroid[] =
    "brave.origin.subscription_link_status_android";
#endif

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace prefs
}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_PREF_NAMES_H_
