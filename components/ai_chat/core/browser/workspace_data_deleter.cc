// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/workspace_data_deleter.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"

namespace ai_chat {

WorkspaceDataDeleter::WorkspaceDataDeleter(AIChatService& service,
                                           Remover remover)
    : service_(service), remover_(std::move(remover)) {
  observation_.Observe(&service);
}

WorkspaceDataDeleter::~WorkspaceDataDeleter() = default;

void WorkspaceDataDeleter::OnWorkspacesReleased(
    const std::set<GURL>& workspace_urls) {
  DeleteIfUnreferenced(workspace_urls);
}

void WorkspaceDataDeleter::OnSavedConversationsDeleted(
    const std::set<GURL>& released_workspace_urls,
    base::optional_ref<const std::set<GURL>> saved_workspace_urls) {
  if (!saved_workspace_urls) {
    // The deletion failed, so the database may still reference them.
    DeleteIfUnreferenced(released_workspace_urls);
    return;
  }
  // Nothing persisted references any workspace now.
  std::set<GURL> candidates = released_workspace_urls;
  candidates.insert(saved_workspace_urls->begin(), saved_workspace_urls->end());
  DeleteIfNotReferencedBy(std::move(candidates), {});
}

void WorkspaceDataDeleter::DeleteIfUnreferenced(std::set<GURL> candidates) {
  if (candidates.empty()) {
    return;
  }
  service_->GetSavedWorkspaceUrls(
      base::BindOnce(&WorkspaceDataDeleter::DeleteIfNotReferencedBy,
                     weak_ptr_factory_.GetWeakPtr(), std::move(candidates)));
}

void WorkspaceDataDeleter::DeleteIfNotReferencedBy(
    std::set<GURL> candidates,
    std::set<GURL> saved_workspace_urls) {
  // Checked now, rather than when the conversation was removed, in case the
  // workspace has since been attached to another conversation.
  const std::set<GURL> in_memory = service_->GetInMemoryWorkspaceUrls();
  for (const GURL& url : candidates) {
    if (in_memory.contains(url) || saved_workspace_urls.contains(url)) {
      continue;
    }
    DVLOG(1) << "Deleting data for unreferenced workspace " << url.spec();
    remover_.Run(url);
  }
}

}  // namespace ai_chat
