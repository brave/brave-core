// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/bignumber_code_plugin.h"

#include "components/grit/brave_components_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace ai_chat {

BigNumberCodePlugin::BigNumberCodePlugin() = default;

BigNumberCodePlugin::~BigNumberCodePlugin() = default;

std::string_view BigNumberCodePlugin::Description() const {
  return "bignumber.js is available in the global scope. Use it for any "
         "decimal math (i.e. financial calculations). "
         "Do not use require to import bignumber.js, as it is not needed.";
}

std::string_view BigNumberCodePlugin::InclusionKeyword() const {
  return "BigNumber";
}

std::string_view BigNumberCodePlugin::SetupScript() {
  if (script_.empty()) {
    script_ = ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
        IDR_AI_CHAT_BIGNUMBER_JS);
  }
  return script_;
}

}  // namespace ai_chat
