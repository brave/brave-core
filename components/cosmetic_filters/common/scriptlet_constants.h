// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_COSMETIC_FILTERS_COMMON_SCRIPTLET_CONSTANTS_H_
#define BRAVE_COMPONENTS_COSMETIC_FILTERS_COMMON_SCRIPTLET_CONSTANTS_H_

namespace cosmetic_filters {

// Defines `scriptletGlobals`, a `Map` wrapped in a `Proxy` that some of uBlock
// Origin's scriptlet resources rely on. Prepended to injected scriptlets on all
// platforms. It contains a single `%s` placeholder for the `Map` constructor
// arguments (`canDebug` used on non-iOS platforms).
constexpr const char kScriptletGlobalsScript[] = R"js(
  const scriptletGlobals = (() => {
    const forwardedMapMethods = ["has", "get", "set"];
    const handler = {
      get(target, prop) {
        if (forwardedMapMethods.includes(prop)) {
          return Map.prototype[prop].bind(target)
        }
        return target.get(prop);
      },
      set(target, prop, value) {
        if (!forwardedMapMethods.includes(prop)) {
          target.set(prop, value);
        }
      }
    };
    return new Proxy(new Map(%s), handler);
  })();
)js";

}  // namespace cosmetic_filters

#endif  // BRAVE_COMPONENTS_COSMETIC_FILTERS_COMMON_SCRIPTLET_CONSTANTS_H_
