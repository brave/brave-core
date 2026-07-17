// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/workspace/workspace_service.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"

namespace ai_chat {

namespace {

using workspace::FileOpResult;

// Canonicalizes the stored root (blocking) so symlink confinement works, then
// dispatches to the matching workspace_file_ops function. Runs on the pool.
base::expected<base::FilePath, std::string> CanonicalRoot(
    const base::FilePath& root) {
  base::FilePath canonical = base::MakeAbsoluteFilePath(root);
  if (canonical.empty()) {
    return base::unexpected("workspace folder is no longer accessible");
  }
  return canonical;
}

FileOpResult ListDirOnSequence(base::FilePath root,
                               std::string relative,
                               int depth) {
  auto canonical = CanonicalRoot(root);
  if (!canonical.has_value()) {
    return base::unexpected(canonical.error());
  }
  return workspace::ListDir(*canonical, relative, depth);
}

FileOpResult ReadFileOnSequence(base::FilePath root,
                                std::string relative,
                                std::optional<int> start_line,
                                std::optional<int> end_line) {
  auto canonical = CanonicalRoot(root);
  if (!canonical.has_value()) {
    return base::unexpected(canonical.error());
  }
  return workspace::ReadFile(*canonical, relative, start_line, end_line);
}

FileOpResult GrepOnSequence(base::FilePath root,
                            std::string relative,
                            std::string pattern,
                            std::string include) {
  auto canonical = CanonicalRoot(root);
  if (!canonical.has_value()) {
    return base::unexpected(canonical.error());
  }
  return workspace::Grep(*canonical, relative, pattern, include);
}

FileOpResult GlobOnSequence(base::FilePath root,
                            std::string relative,
                            std::string pattern) {
  auto canonical = CanonicalRoot(root);
  if (!canonical.has_value()) {
    return base::unexpected(canonical.error());
  }
  return workspace::Glob(*canonical, relative, pattern);
}

FileOpResult RepoMapOnSequence(base::FilePath root, std::string relative) {
  auto canonical = CanonicalRoot(root);
  if (!canonical.has_value()) {
    return base::unexpected(canonical.error());
  }
  return workspace::RepoMap(*canonical, relative);
}

workspace::WriteResult CreateFileOnSequence(base::FilePath root,
                                            std::string relative,
                                            std::string content) {
  auto canonical = CanonicalRoot(root);
  if (!canonical.has_value()) {
    return base::unexpected(canonical.error());
  }
  return workspace::CreateFile(*canonical, relative, content);
}

workspace::WriteResult StrReplaceOnSequence(base::FilePath root,
                                            std::string relative,
                                            std::string old_str,
                                            std::string new_str) {
  auto canonical = CanonicalRoot(root);
  if (!canonical.has_value()) {
    return base::unexpected(canonical.error());
  }
  return workspace::StrReplace(*canonical, relative, old_str, new_str);
}

workspace::WriteResult InsertOnSequence(base::FilePath root,
                                        std::string relative,
                                        int insert_line,
                                        std::string text) {
  auto canonical = CanonicalRoot(root);
  if (!canonical.has_value()) {
    return base::unexpected(canonical.error());
  }
  return workspace::Insert(*canonical, relative, insert_line, text);
}

workspace::WriteResult AppendFileOnSequence(base::FilePath root,
                                            std::string relative,
                                            std::string content) {
  auto canonical = CanonicalRoot(root);
  if (!canonical.has_value()) {
    return base::unexpected(canonical.error());
  }
  return workspace::AppendFile(*canonical, relative, content);
}

}  // namespace

WorkspaceService::UndoEntry::UndoEntry() = default;
WorkspaceService::UndoEntry::UndoEntry(
    std::string relative,
    base::FilePath resolved_path,
    std::optional<std::string> previous_content)
    : relative(std::move(relative)),
      resolved_path(std::move(resolved_path)),
      previous_content(std::move(previous_content)) {}
WorkspaceService::UndoEntry::UndoEntry(UndoEntry&&) = default;
WorkspaceService::UndoEntry& WorkspaceService::UndoEntry::operator=(
    UndoEntry&&) = default;
WorkspaceService::UndoEntry::~UndoEntry() = default;

WorkspaceService::WorkspaceService()
    : task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE})) {}

WorkspaceService::~WorkspaceService() = default;

void WorkspaceService::SetWorkspaceRoot(const std::string& conversation_id,
                                        const base::FilePath& root) {
  roots_[conversation_id] = root;
}

void WorkspaceService::ClearWorkspace(const std::string& conversation_id) {
  roots_.erase(conversation_id);
}

std::optional<base::FilePath> WorkspaceService::GetWorkspaceRoot(
    const std::string& conversation_id) const {
  auto it = roots_.find(conversation_id);
  if (it == roots_.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool WorkspaceService::HasWorkspace(const std::string& conversation_id) const {
  return roots_.contains(conversation_id);
}

void WorkspaceService::SetWritesAllowed(const std::string& conversation_id,
                                        bool allowed) {
  if (allowed) {
    writes_allowed_.insert(conversation_id);
  } else {
    writes_allowed_.erase(conversation_id);
  }
}

bool WorkspaceService::AreWritesAllowed(
    const std::string& conversation_id) const {
  return writes_allowed_.contains(conversation_id);
}

std::optional<base::FilePath> WorkspaceService::RootOrReportError(
    const std::string& conversation_id,
    ResultCallback& callback) {
  std::optional<base::FilePath> root = GetWorkspaceRoot(conversation_id);
  if (!root) {
    std::move(callback).Run(
        base::unexpected("no workspace folder is open for this conversation"));
  }
  return root;
}

void WorkspaceService::ListDir(const std::string& conversation_id,
                               const std::string& relative,
                               int depth,
                               ResultCallback callback) {
  std::optional<base::FilePath> root =
      RootOrReportError(conversation_id, callback);
  if (!root) {
    return;
  }
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&ListDirOnSequence, *root, relative, depth),
      std::move(callback));
}

void WorkspaceService::ReadFile(const std::string& conversation_id,
                                const std::string& relative,
                                std::optional<int> start_line,
                                std::optional<int> end_line,
                                ResultCallback callback) {
  std::optional<base::FilePath> root =
      RootOrReportError(conversation_id, callback);
  if (!root) {
    return;
  }
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ReadFileOnSequence, *root, relative, start_line,
                     end_line),
      std::move(callback));
}

void WorkspaceService::Grep(const std::string& conversation_id,
                            const std::string& relative,
                            const std::string& pattern,
                            const std::string& include,
                            ResultCallback callback) {
  std::optional<base::FilePath> root =
      RootOrReportError(conversation_id, callback);
  if (!root) {
    return;
  }
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&GrepOnSequence, *root, relative, pattern, include),
      std::move(callback));
}

void WorkspaceService::Glob(const std::string& conversation_id,
                            const std::string& relative,
                            const std::string& pattern,
                            ResultCallback callback) {
  std::optional<base::FilePath> root =
      RootOrReportError(conversation_id, callback);
  if (!root) {
    return;
  }
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&GlobOnSequence, *root, relative, pattern),
      std::move(callback));
}

void WorkspaceService::RepoMap(const std::string& conversation_id,
                               const std::string& relative,
                               ResultCallback callback) {
  std::optional<base::FilePath> root =
      RootOrReportError(conversation_id, callback);
  if (!root) {
    return;
  }
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&RepoMapOnSequence, *root, relative),
      std::move(callback));
}

void WorkspaceService::CreateFile(const std::string& conversation_id,
                                  const std::string& relative,
                                  const std::string& content,
                                  ResultCallback callback) {
  std::optional<base::FilePath> root =
      RootOrReportError(conversation_id, callback);
  if (!root) {
    return;
  }
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&CreateFileOnSequence, *root, relative, content),
      base::BindOnce(&WorkspaceService::OnWriteComplete,
                     weak_factory_.GetWeakPtr(), conversation_id, relative,
                     std::move(callback)));
}

void WorkspaceService::StrReplace(const std::string& conversation_id,
                                  const std::string& relative,
                                  const std::string& old_str,
                                  const std::string& new_str,
                                  ResultCallback callback) {
  std::optional<base::FilePath> root =
      RootOrReportError(conversation_id, callback);
  if (!root) {
    return;
  }
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&StrReplaceOnSequence, *root, relative, old_str, new_str),
      base::BindOnce(&WorkspaceService::OnWriteComplete,
                     weak_factory_.GetWeakPtr(), conversation_id, relative,
                     std::move(callback)));
}

void WorkspaceService::Insert(const std::string& conversation_id,
                              const std::string& relative,
                              int insert_line,
                              const std::string& text,
                              ResultCallback callback) {
  std::optional<base::FilePath> root =
      RootOrReportError(conversation_id, callback);
  if (!root) {
    return;
  }
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&InsertOnSequence, *root, relative, insert_line, text),
      base::BindOnce(&WorkspaceService::OnWriteComplete,
                     weak_factory_.GetWeakPtr(), conversation_id, relative,
                     std::move(callback)));
}

void WorkspaceService::AppendFile(const std::string& conversation_id,
                                  const std::string& relative,
                                  const std::string& content,
                                  ResultCallback callback) {
  std::optional<base::FilePath> root =
      RootOrReportError(conversation_id, callback);
  if (!root) {
    return;
  }
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&AppendFileOnSequence, *root, relative, content),
      base::BindOnce(&WorkspaceService::OnWriteComplete,
                     weak_factory_.GetWeakPtr(), conversation_id, relative,
                     std::move(callback)));
}

void WorkspaceService::UndoEdit(const std::string& conversation_id,
                                const std::string& relative,
                                ResultCallback callback) {
  auto stack_it = undo_stacks_.find(conversation_id);
  if (stack_it != undo_stacks_.end()) {
    std::vector<UndoEntry>& stack = stack_it->second;
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
      if (it->relative != relative) {
        continue;
      }
      base::FilePath resolved_path = it->resolved_path;
      std::optional<std::string> previous_content = it->previous_content;
      // Erase this entry (convert reverse iterator to forward iterator).
      stack.erase(std::next(it).base());
      task_runner_->PostTaskAndReplyWithResult(
          FROM_HERE,
          base::BindOnce(&workspace::RestoreFile, std::move(resolved_path),
                         std::move(previous_content)),
          std::move(callback));
      return;
    }
  }
  std::move(callback).Run(
      base::unexpected("nothing to undo for " + relative));
}

void WorkspaceService::OnWriteComplete(const std::string& conversation_id,
                                       const std::string& relative,
                                       ResultCallback callback,
                                       workspace::WriteResult result) {
  if (!result.has_value()) {
    std::move(callback).Run(base::unexpected(result.error()));
    return;
  }
  undo_stacks_[conversation_id].emplace_back(
      relative, result->resolved_path, std::move(result->previous_content));
  std::move(callback).Run(FileOpResult(result->message));
}

base::WeakPtr<WorkspaceService> WorkspaceService::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace ai_chat
