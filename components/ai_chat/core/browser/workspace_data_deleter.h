// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_DATA_DELETER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_DATA_DELETER_H_

#include <optional>
#include <set>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/types/optional_ref.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "url/gurl.h"

namespace ai_chat {

// Deletes a workspace's stored data (site data, storage, permissions) once no
// conversation of |service|, in memory or persisted, references it any more.
// Owned by |service|.
class WorkspaceDataDeleter : public AIChatService::Observer {
 public:
  // Deletes the data of the workspace at the given workspace://<uuid> URL.
  using Remover = base::RepeatingCallback<void(const GURL&)>;

  WorkspaceDataDeleter(AIChatService& service, Remover remover);
  ~WorkspaceDataDeleter() override;
  WorkspaceDataDeleter(const WorkspaceDataDeleter&) = delete;
  WorkspaceDataDeleter& operator=(const WorkspaceDataDeleter&) = delete;

  // AIChatService::Observer:
  void OnWorkspacesReleased(const std::set<GURL>& workspace_urls) override;
  void OnSavedConversationsDeleted(
      const std::set<GURL>& released_workspace_urls,
      base::optional_ref<const std::set<GURL>> saved_workspace_urls) override;

 private:
  // Deletes every workspace in |candidates| that no conversation references,
  // in memory or persisted.
  void DeleteIfUnreferenced(std::set<GURL> candidates);
  // Deletes every workspace in |candidates| that neither |saved_workspace_urls|
  // nor any in-memory conversation references.
  void DeleteIfNotReferencedBy(std::set<GURL> candidates,
                               std::set<GURL> saved_workspace_urls);

  const raw_ref<AIChatService> service_;
  Remover remover_;
  base::ScopedObservation<AIChatService, AIChatService::Observer> observation_{
      this};
  base::WeakPtrFactory<WorkspaceDataDeleter> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_DATA_DELETER_H_
