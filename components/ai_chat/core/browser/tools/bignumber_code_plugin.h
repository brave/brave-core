// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_BIGNUMBER_CODE_PLUGIN_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_BIGNUMBER_CODE_PLUGIN_H_

#include <string>
#include <string_view>

#include "brave/components/ai_chat/core/browser/tools/code_plugin.h"

namespace ai_chat {

// Plugin that provides BigNumber.js for precise decimal arithmetic
// in the code execution sandbox.
class BigNumberCodePlugin : public CodePlugin {
 public:
  BigNumberCodePlugin();
  ~BigNumberCodePlugin() override;

  BigNumberCodePlugin(const BigNumberCodePlugin&) = delete;
  BigNumberCodePlugin& operator=(const BigNumberCodePlugin&) = delete;

  // CodePlugin implementation
  std::string_view Description() const override;
  std::string_view InclusionKeyword() const override;
  std::string_view SetupScript() const override;
  std::string_view ArtifactType() const override;

 private:
  mutable std::string script_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_BIGNUMBER_CODE_PLUGIN_H_
