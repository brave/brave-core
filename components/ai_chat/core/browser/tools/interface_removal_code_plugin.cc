// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/interface_removal_code_plugin.h"

#include "components/grit/brave_components_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace ai_chat {

InterfaceRemovalCodePlugin::InterfaceRemovalCodePlugin() = default;

InterfaceRemovalCodePlugin::~InterfaceRemovalCodePlugin() = default;

std::string_view InterfaceRemovalCodePlugin::Description() const {
  return "";
}

std::string_view InterfaceRemovalCodePlugin::InclusionKeyword() const {
  return "";
}

std::string_view InterfaceRemovalCodePlugin::SetupScript() {
  if (script_.empty()) {
    script_ = ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
        IDR_AI_CHAT_INTERFACE_REMOVAL_JS);
  }
  return script_;
}

}  // namespace ai_chat
