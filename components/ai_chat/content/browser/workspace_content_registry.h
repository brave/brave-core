// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_REGISTRY_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_REGISTRY_H_

#include <map>
#include <string>

#include "base/files/file_path.h"
#include "base/sequence_checker.h"
#include "base/supports_user_data.h"

namespace content {
class BrowserContext;
}

namespace ai_chat {

// Per-BrowserContext map of workspace uuid -> the local folder that workspace
// is attached to. WorkspaceAssociatedContent registers itself here for as long
// as it lives, so that LeoWorkspaceContentUI can serve the folder's files at
// chrome-untrusted://leo-workspace-content/<uuid>/<path>.
//
// Keeping this keyed by uuid (rather than serving a path directly) means a
// stale iframe pointing at a conversation that has gone away resolves to
// nothing, instead of resolving against whichever folder is current.
//
// UI thread only.
class WorkspaceContentRegistry : public base::SupportsUserData::Data {
 public:
  WorkspaceContentRegistry();
  ~WorkspaceContentRegistry() override;

  WorkspaceContentRegistry(const WorkspaceContentRegistry&) = delete;
  WorkspaceContentRegistry& operator=(const WorkspaceContentRegistry&) = delete;

  static WorkspaceContentRegistry* GetOrCreate(
      content::BrowserContext* browser_context);

  void Register(const std::string& uuid, const base::FilePath& folder);
  void Unregister(const std::string& uuid);

  // Returns an empty path when |uuid| is not registered.
  base::FilePath GetFolder(const std::string& uuid) const;

 private:
  std::map<std::string, base::FilePath> folders_;
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_REGISTRY_H_
