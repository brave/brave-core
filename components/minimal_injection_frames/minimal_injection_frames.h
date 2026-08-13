/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MINIMAL_INJECTION_FRAMES_MINIMAL_INJECTION_FRAMES_H_
#define BRAVE_COMPONENTS_MINIMAL_INJECTION_FRAMES_MINIMAL_INJECTION_FRAMES_H_

namespace url {
class Origin;
}  // namespace url

namespace brave {

// Returns true for origins that only host a small embedded widget, where the
// optional per-frame injections below are all no-ops but stay observable by the
// widget's own script. Callers must still apply their own feature and pref
// checks.
bool IsMinimalInjectionFrame(const url::Origin& origin);

}  // namespace brave

#endif  // BRAVE_COMPONENTS_MINIMAL_INJECTION_FRAMES_MINIMAL_INJECTION_FRAMES_H_
