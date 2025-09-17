// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/code_sandbox_script_storage.h"

#include "base/memory/ref_counted_memory.h"
#include "base/no_destructor.h"
#include "base/unguessable_token.h"

namespace ai_chat {

CodeSandboxScriptStorage* CodeSandboxScriptStorage::GetInstance() {
  static base::NoDestructor<CodeSandboxScriptStorage> instance;
  return instance.get();
}

CodeSandboxScriptStorage::CodeSandboxScriptStorage() = default;
CodeSandboxScriptStorage::~CodeSandboxScriptStorage() = default;

std::string CodeSandboxScriptStorage::StoreScript(std::string script) {
  auto request_id = base::UnguessableToken::Create().ToString();
  scripts_[request_id] =
      base::MakeRefCounted<base::RefCountedString>(std::move(script));
  return request_id;
}

scoped_refptr<base::RefCountedString> CodeSandboxScriptStorage::ConsumeScript(
    std::string_view request_id) {
  auto it = scripts_.find(request_id);
  if (it != scripts_.end()) {
    auto script = std::move(it->second);
    scripts_.erase(it);
    return script;
  }
  return nullptr;
}

}  // namespace ai_chat
