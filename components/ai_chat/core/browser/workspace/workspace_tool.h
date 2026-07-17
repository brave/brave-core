// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_WORKSPACE_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_WORKSPACE_TOOL_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/workspace/workspace_file_ops.h"

namespace ai_chat {

class WorkspaceService;

// A single "workspace" file tool exposed to the assistant. It is a thin proxy:
// all real work happens in WorkspaceService (native file I/O, confined to the
// conversation's selected root). One generic class is parameterized by `op` and
// its schema rather than a subclass per tool, since the C++ side only forwards
// arguments (cf. ContentTool). Read ops self-execute; write ops (added later)
// set `requires_permission` so the user must approve before UseTool runs.
class WorkspaceTool : public Tool {
 public:
  enum class Op {
    kListDir,
    kReadFile,
    kGrep,
    kGlob,
    kRepoMap,
    kCreateFile,
    kStrReplace,
    kInsert,
    kAppendFile,
    kUndoEdit,
  };

  WorkspaceTool(Op op,
                std::string name,
                std::string description,
                std::optional<base::DictValue> input_properties,
                std::optional<std::vector<std::string>> required,
                bool requires_permission,
                base::WeakPtr<WorkspaceService> service,
                std::string conversation_id);
  ~WorkspaceTool() override;

  WorkspaceTool(const WorkspaceTool&) = delete;
  WorkspaceTool& operator=(const WorkspaceTool&) = delete;

  // Tool:
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::DictValue> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  std::variant<bool, mojom::PermissionChallengePtr>
  RequiresUserInteractionBeforeHandling(
      const mojom::ToolUseEvent& tool_use) const override;
  void UserPermissionGranted(const std::string& tool_use_id) override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  void OnResult(UseToolCallback callback, workspace::FileOpResult result);

  Op op_;
  std::string name_;
  std::string description_;
  std::optional<base::DictValue> input_properties_;
  std::optional<std::vector<std::string>> required_;
  bool requires_permission_;
  bool permission_granted_ = false;
  base::WeakPtr<WorkspaceService> service_;
  std::string conversation_id_;
  base::WeakPtrFactory<WorkspaceTool> weak_factory_{this};
};

// Builds the current set of workspace tools bound to `service` and
// `conversation_id`. The returned tools are owned by the caller (the
// WorkspaceToolProvider).
std::vector<std::unique_ptr<WorkspaceTool>> BuildWorkspaceTools(
    base::WeakPtr<WorkspaceService> service,
    const std::string& conversation_id);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_WORKSPACE_TOOL_H_
