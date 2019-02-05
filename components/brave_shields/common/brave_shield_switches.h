/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_BRAVE_SHIELD_SWITCHES_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_BRAVE_SHIELD_SWITCHES_H_

namespace brave_shields {
namespace switches {

// The following command-line switches allow to override defaults of
// corresponding shields. ATTENTION: using any of this switch persists
// a given value in content settings of any loaded user profile. Switches
// behavior mirrors the behavior of toggles on brave://settings/shields.
//
// Example: --set-shields-ads-default=block

constexpr char kShieldsAdsSetDefault[] = "set-shields-ads-default";

constexpr char kShieldsHttpseSetDefault[] = "set-shields-httpse-default";

constexpr char kShieldsNoScriptSetDefault[] =
    "set-shields-noscript-default";

constexpr char kShieldsFingerprintingSetDefault[] =
    "set-shields-fingerprinting-default";

constexpr char kShieldsSetDefault[] = "set-shields-default";

constexpr char kShieldsCookiePolicySetDefault[] =
    "set-shields-cookie-policy-default";

constexpr char kBlock[] = "block";
constexpr char kBlock3rd[] = "block3rd";
constexpr char kAllow[] = "allow";

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_BRAVE_SHIELD_SWITCHES_H_

}  // namespace switches
}  // namespace brave_shields
