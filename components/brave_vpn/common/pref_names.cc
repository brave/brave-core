/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/pref_names.h"

namespace brave_vpn {
namespace prefs {

#if BUILDFLAG(IS_ANDROID)
const char kBraveVPNPurchaseTokenAndroid[] =
    "brave.brave_vpn.purchase_token_android";
const char kBraveVPNPackageAndroid[] = "brave.brave_vpn.package_android";
const char kBraveVPNProductIdAndroid[] = "brave.brave_vpn.product_id_android";
#endif

}  // namespace prefs

}  // namespace brave_vpn
