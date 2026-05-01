// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_FARBLING_ARGS_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_FARBLING_ARGS_H_

#include "base/values.h"

namespace base {
class Token;
}  // namespace base

namespace brave_shields {

// Generates the per-origin farbling arguments expected by farbling.ts (the
// `FarblingArgs` interface). The values are derived from a seeded pseudo-random
// generator seeded from `farbling_token`
base::DictValue MakeFarblingArgs(const base::Token& farbling_token);

}  // namespace brave_shields

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_FARBLING_ARGS_H_
