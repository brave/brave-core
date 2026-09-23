// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_COSMETIC_FILTERS_COMMON_SCRIPTLET_CONSTANTS_H_
#define BRAVE_COMPONENTS_COSMETIC_FILTERS_COMMON_SCRIPTLET_CONSTANTS_H_

#include <string>

namespace cosmetic_filters {

// Builds the globals block that is prepended to injected scriptlets on all
// platforms. It defines:
//   - `scriptletGlobals`, a `Map` wrapped in a `Proxy` that some of uBlock
//     Origin's scriptlet resources rely on.
//   - `deAmpEnabled`, used by the de-amp scriptlet resource.
//
// `is_de_amp_enabled` toggles de-amping. `can_debug` enables uBlock Origin's
// scriptlet debug logging and is only set on non-iOS platforms.
std::string GetScriptletGlobalsScript(bool is_de_amp_enabled, bool can_debug);

}  // namespace cosmetic_filters

#endif  // BRAVE_COMPONENTS_COSMETIC_FILTERS_COMMON_SCRIPTLET_CONSTANTS_H_
