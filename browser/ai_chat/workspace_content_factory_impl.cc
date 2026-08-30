// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/workspace_content_factory_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/browser/brave_tab_helpers.h"
#include "brave/components/ai_chat/content/browser/workspace_associated_content.h"
#include "content/public/browser/browser_context.h"

namespace ai_chat {

WorkspaceContentFactoryImpl::WorkspaceContentFactoryImpl(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {}

WorkspaceContentFactoryImpl::~WorkspaceContentFactoryImpl() = default;

void WorkspaceContentFactoryImpl::CreateWorkspaceContent(
    const base::FilePath& folder_path,
    std::optional<std::string> uuid,
    CreateWorkspaceContentCallback callback) {
  // A restored workspace names a folder picked in the past, which may since
  // have moved or been deleted. Check first, so a stale conversation doesn't
  // get a handle to a path that no longer means what the user chose.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&base::DirectoryExists, folder_path),
      base::BindOnce(&WorkspaceContentFactoryImpl::OnFolderChecked,
                     weak_ptr_factory_.GetWeakPtr(), folder_path,
                     std::move(uuid), std::move(callback)));
}

void WorkspaceContentFactoryImpl::OnFolderChecked(
    base::FilePath folder_path,
    std::optional<std::string> uuid,
    CreateWorkspaceContentCallback callback,
    bool folder_exists) {
  if (!folder_exists) {
    DVLOG(1) << __func__ << " workspace folder is gone: " << folder_path;
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(std::make_unique<WorkspaceAssociatedContent>(
      std::move(folder_path), std::move(uuid), browser_context_,
      base::BindOnce(&brave::AttachPrivacySensitiveTabHelpers)));
}

}  // namespace ai_chat
