// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_INTERFACE_REMOVAL_CODE_PLUGIN_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_INTERFACE_REMOVAL_CODE_PLUGIN_H_

#include <string>
#include <string_view>

#include "brave/components/ai_chat/core/browser/tools/code_plugin.h"

namespace ai_chat {

// Plugin that removes unnecessary JS interfaces from the code execution
// sandbox for security purposes.
class InterfaceRemovalCodePlugin : public CodePlugin {
 public:
  InterfaceRemovalCodePlugin();
  ~InterfaceRemovalCodePlugin() override;

  InterfaceRemovalCodePlugin(const InterfaceRemovalCodePlugin&) = delete;
  InterfaceRemovalCodePlugin& operator=(const InterfaceRemovalCodePlugin&) =
      delete;

  // CodePlugin implementation
  std::string_view Description() const override;
  std::string_view InclusionKeyword() const override;
  std::string_view SetupScript() override;

 private:
  std::string script_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_INTERFACE_REMOVAL_CODE_PLUGIN_H_
