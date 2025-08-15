// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_script_utils.h"

#include "base/json/json_writer.h"
#include "base/strings/strcat.h"

namespace psst {

std::string GetScriptWithParams(const std::string& script,
                                std::optional<base::Value> params) {
  if (!params) {
    return script;
  }

  const auto* params_dict = params->GetIfDict();
  if (!params_dict) {
    return script;
  }

  std::optional<std::string> params_json = base::WriteJsonWithOptions(
      *params_dict, base::JSONWriter::OPTIONS_PRETTY_PRINT);
  if (!params_json) {
    return script;
  }

  std::string result =
      base::StrCat({"const params = ", *params_json, ";\n", script});
  return result;
}

}  // namespace psst
