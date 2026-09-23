// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/cosmetic_filters/common/scriptlet_constants.h"

#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace cosmetic_filters {

namespace {

// Defines `scriptletGlobals`, a `Map` wrapped in a `Proxy` that some of uBlock
// Origin's scriptlet resources rely on. It contains a single `%s` placeholder
// for the `Map` constructor arguments (`canDebug` used on non-iOS platforms).
constexpr char kScriptletGlobalsScript[] = R"js(
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
  let deAmpEnabled = %s;
)js";

}  // namespace

std::string GetScriptletGlobalsScript(bool is_de_amp_enabled, bool can_debug) {
  return absl::StrFormat(kScriptletGlobalsScript,
                         can_debug ? R"([["canDebug", true]])" : "",
                         is_de_amp_enabled ? "true" : "false");
}

}  // namespace cosmetic_filters
