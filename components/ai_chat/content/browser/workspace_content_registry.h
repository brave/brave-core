// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_REGISTRY_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_REGISTRY_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/supports_user_data.h"

namespace content {
class BrowserContext;
}

namespace ai_chat {

// Per-BrowserContext map of workspace uuid -> the local folder that workspace
// is attached to, so that the leo-workspace WebUI data source can serve the
// folder's files at chrome-untrusted://leo-workspace/<uuid>/files/<path>.
//
// Keying by uuid scopes a request to one workspace's folder, but is not an
// isolation boundary: all workspaces share one host, and so one origin.
// TODO(https://github.com/brave/brave-browser/issues/58792): Support dynamic
// origins from WebUI, so each workspace's files get an origin of their own.
//
// UI thread only.
class WorkspaceContentRegistry : public base::SupportsUserData::Data {
 public:
  WorkspaceContentRegistry();
  ~WorkspaceContentRegistry() override;

  WorkspaceContentRegistry(const WorkspaceContentRegistry&) = delete;
  WorkspaceContentRegistry& operator=(const WorkspaceContentRegistry&) = delete;

  // Creates the registry on first use; owned by |browser_context| and never
  // null. Registration paths only: reads use Get(), so that a renderer-driven
  // request cannot bring a registry into existence.
  static WorkspaceContentRegistry* GetOrCreate(
      content::BrowserContext* browser_context);

  // Returns null when nothing has registered for |browser_context| yet.
  static WorkspaceContentRegistry* Get(
      content::BrowserContext* browser_context);

  // Serves |folder| until the returned closure runs; the closure may outlive
  // the registry.
  [[nodiscard]] base::ScopedClosureRunner Register(
      const std::string& uuid,
      const base::FilePath& folder);

  // Null when |uuid| is not registered. Invalidated by the next
  // Register()/Unregister().
  const base::FilePath* GetFolder(const std::string& uuid) const;

 private:
  void Unregister(const std::string& uuid);

  base::flat_map<std::string, base::FilePath> folders_;
  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<WorkspaceContentRegistry> weak_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_REGISTRY_H_
