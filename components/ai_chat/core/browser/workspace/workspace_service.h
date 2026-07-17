// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_WORKSPACE_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_WORKSPACE_SERVICE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/workspace/workspace_file_ops.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace ai_chat {

// Profile-scoped store of each conversation's selected "workspace" root
// directory, plus the entry point for running the local file tools against it.
// The user picks a folder (native dialog, browser layer) which is recorded via
// SetWorkspaceRoot(); WorkspaceTool then calls the read/search ops here, which
// run on a blocking background sequence and are confined to the root (see
// workspace_file_ops.h). Depends only on //base so it can live in core/browser.
class WorkspaceService : public KeyedService {
 public:
  using ResultCallback =
      base::OnceCallback<void(workspace::FileOpResult result)>;

  WorkspaceService();
  ~WorkspaceService() override;

  WorkspaceService(const WorkspaceService&) = delete;
  WorkspaceService& operator=(const WorkspaceService&) = delete;

  // Root management (synchronous; no blocking I/O).
  void SetWorkspaceRoot(const std::string& conversation_id,
                        const base::FilePath& root);
  void ClearWorkspace(const std::string& conversation_id);
  std::optional<base::FilePath> GetWorkspaceRoot(
      const std::string& conversation_id) const;
  bool HasWorkspace(const std::string& conversation_id) const;

  // When allowed, the workspace write tools run without a per-call permission
  // prompt for this conversation. Off by default.
  void SetWritesAllowed(const std::string& conversation_id, bool allowed);
  bool AreWritesAllowed(const std::string& conversation_id) const;

  // File operations. Each resolves the conversation's root, then runs the
  // corresponding workspace_file_ops function on a background sequence, and
  // replies on the caller's sequence. If the conversation has no workspace the
  // callback runs with an error result.
  void ListDir(const std::string& conversation_id,
               const std::string& relative,
               int depth,
               ResultCallback callback);
  void ReadFile(const std::string& conversation_id,
                const std::string& relative,
                std::optional<int> start_line,
                std::optional<int> end_line,
                ResultCallback callback);
  void Grep(const std::string& conversation_id,
            const std::string& relative,
            const std::string& pattern,
            const std::string& include,
            ResultCallback callback);
  void Glob(const std::string& conversation_id,
            const std::string& relative,
            const std::string& pattern,
            ResultCallback callback);
  void RepoMap(const std::string& conversation_id,
               const std::string& relative,
               ResultCallback callback);

  // Mutating operations. Each records undo state on success so a subsequent
  // UndoEdit can revert it.
  void CreateFile(const std::string& conversation_id,
                  const std::string& relative,
                  const std::string& content,
                  ResultCallback callback);
  void StrReplace(const std::string& conversation_id,
                  const std::string& relative,
                  const std::string& old_str,
                  const std::string& new_str,
                  ResultCallback callback);
  void Insert(const std::string& conversation_id,
              const std::string& relative,
              int insert_line,
              const std::string& text,
              ResultCallback callback);
  void AppendFile(const std::string& conversation_id,
                  const std::string& relative,
                  const std::string& content,
                  ResultCallback callback);
  // Reverts the most recent edit to `relative` for this conversation.
  void UndoEdit(const std::string& conversation_id,
                const std::string& relative,
                ResultCallback callback);

  base::WeakPtr<WorkspaceService> GetWeakPtr();

 private:
  // Records enough to revert one edit: the file's path and its prior contents
  // (std::nullopt if the edit created the file, so undo deletes it).
  struct UndoEntry {
    UndoEntry();
    UndoEntry(std::string relative,
              base::FilePath resolved_path,
              std::optional<std::string> previous_content);
    UndoEntry(UndoEntry&&);
    UndoEntry& operator=(UndoEntry&&);
    ~UndoEntry();

    std::string relative;
    base::FilePath resolved_path;
    std::optional<std::string> previous_content;
  };

  // Resolves the conversation's root, or runs `callback` with an error and
  // returns nullopt if there isn't one.
  std::optional<base::FilePath> RootOrReportError(
      const std::string& conversation_id,
      ResultCallback& callback);

  // Reply handler for the mutating ops: records undo state on success and
  // forwards the model-facing message.
  void OnWriteComplete(const std::string& conversation_id,
                       const std::string& relative,
                       ResultCallback callback,
                       workspace::WriteResult result);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::flat_map<std::string, base::FilePath> roots_;
  base::flat_set<std::string> writes_allowed_;
  base::flat_map<std::string, std::vector<UndoEntry>> undo_stacks_;
  base::WeakPtrFactory<WorkspaceService> weak_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_WORKSPACE_SERVICE_H_
