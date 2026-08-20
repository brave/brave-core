/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_USER_AGENT_COMMON_BRAND_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_USER_AGENT_COMMON_BRAND_NAMES_H_

namespace brave_user_agent {

// Brand names as they appear in navigator.userAgentData brand lists. Sites on
// the brave-checks.txt list report kGoogleChromeBrand instead of kBraveBrand.
// Note the Sec-CH-UA header rewrite matches on quoted variants of these, see
// brave/browser/net/brave_user_agent_network_delegate_helper.cc.
inline constexpr char kBraveBrand[] = "Brave";
inline constexpr char kGoogleChromeBrand[] = "Google Chrome";

}  // namespace brave_user_agent

#endif  // BRAVE_COMPONENTS_BRAVE_USER_AGENT_COMMON_BRAND_NAMES_H_
