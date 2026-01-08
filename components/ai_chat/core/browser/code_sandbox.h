// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CODE_SANDBOX_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CODE_SANDBOX_H_

#include <string>

#include "base/functional/callback_forward.h"

namespace ai_chat {

// Interface for executing code in a sandboxed environment.
class CodeSandbox {
 public:
  using ExecuteCodeCallback = base::OnceCallback<void(std::string output)>;

  virtual ~CodeSandbox() = default;

  // Executes the provided script in a sandboxed environment and returns
  // console output via the callback.
  virtual void ExecuteCode(const std::string& script,
                           ExecuteCodeCallback callback) = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CODE_SANDBOX_H_
