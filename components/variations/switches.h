// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_VARIATIONS_SWITCHES_H_
#define BRAVE_COMPONENTS_VARIATIONS_SWITCHES_H_

namespace variations::switches {

// If this flag is set to a brave/brave-variations pull request number, the
// browser will point "--variations-server-url" to a test seed URL from this
// pull request.
inline constexpr char kVariationsPR[] = "variations-pr";

}  // namespace variations::switches

#endif  // BRAVE_COMPONENTS_VARIATIONS_SWITCHES_H_
