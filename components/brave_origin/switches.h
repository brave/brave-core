/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_SWITCHES_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_SWITCHES_H_

namespace brave_origin::switches {

// Skips the Brave Origin startup dialog. Only honored on Linux, where
// Brave Origin can be used without a purchase.
inline constexpr char kSkipOriginStartupDialog[] = "skip-origin-startup-dialog";

}  // namespace brave_origin::switches

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_SWITCHES_H_
