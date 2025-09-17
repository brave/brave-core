// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CODE_SANDBOX_SCRIPT_STORAGE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CODE_SANDBOX_SCRIPT_STORAGE_H_

#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"

namespace base {
template <typename T>
class NoDestructor;
}

namespace ai_chat {

// Singleton class to manage script storage for code execution requests
class CodeSandboxScriptStorage {
 public:
  static CodeSandboxScriptStorage* GetInstance();

  CodeSandboxScriptStorage(const CodeSandboxScriptStorage&) = delete;
  CodeSandboxScriptStorage& operator=(const CodeSandboxScriptStorage&) = delete;

  // Store the script and return a new request ID
  std::string StoreScript(std::string script);
  scoped_refptr<base::RefCountedString> ConsumeScript(
      std::string_view request_id);

 private:
  friend class base::NoDestructor<CodeSandboxScriptStorage>;

  CodeSandboxScriptStorage();
  ~CodeSandboxScriptStorage();

  base::flat_map<std::string, scoped_refptr<base::RefCountedString>> scripts_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CODE_SANDBOX_SCRIPT_STORAGE_H_
