// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/cosmetic_filters/common/scriptlet_constants.h"

#include "base/check.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/cosmetic_filters/resources/grit/cosmetic_filters_scriptlet_globals_generated.h"
#include "ui/base/resource/resource_bundle.h"

namespace cosmetic_filters {

namespace {

constexpr char kScriptletGlobalsConfigPlaceholder[] =
    "__BRAVE_SCRIPTLET_GLOBALS_CONFIG__";

}  // namespace

std::string GetScriptletGlobalsScript(bool is_de_amp_enabled, bool can_debug) {
  std::string script =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_COSMETIC_FILTERS_SCRIPTLET_GLOBALS_SCRIPTLET_GLOBALS_BUNDLE_JS);
  CHECK_NE(script.find(kScriptletGlobalsConfigPlaceholder), std::string::npos);
  base::ReplaceSubstringsAfterOffset(
      &script, 0, kScriptletGlobalsConfigPlaceholder,
      base::StrCat({"[", can_debug ? "true" : "false", ",",
                    is_de_amp_enabled ? "true" : "false", "]"}));
  return script;
}

}  // namespace cosmetic_filters
