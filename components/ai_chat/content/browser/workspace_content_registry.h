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
// is attached to. WorkspaceAssociatedContent registers itself here for as long
// as it lives, so that the workspace content URLLoaderFactory can serve the
// folder's files at brave-leo-workspace://<uuid>/<path>.
//
// Keying by uuid means a stale iframe pointing at a closed conversation
// resolves to nothing rather than to the current folder. That is a correctness
// property, not an isolation one - isolation comes from each uuid being a
// separate origin.
//
// UI thread only.
class WorkspaceContentRegistry : public base::SupportsUserData::Data {
 public:
  WorkspaceContentRegistry();
  ~WorkspaceContentRegistry() override;

  WorkspaceContentRegistry(const WorkspaceContentRegistry&) = delete;
  WorkspaceContentRegistry& operator=(const WorkspaceContentRegistry&) = delete;

  // Creates the registry on first use. Owned by |browser_context|; never
  // returns null. Call this from registration paths only - reads should use
  // Get(), so that a renderer-driven request cannot bring a registry into
  // existence as a side effect.
  static WorkspaceContentRegistry* GetOrCreate(
      content::BrowserContext* browser_context);

  // Returns null when nothing has registered for |browser_context| yet.
  static WorkspaceContentRegistry* Get(
      content::BrowserContext* browser_context);

  // Serves |folder| at brave-leo-workspace://<uuid>/ until the returned closure
  // runs. Safe to hold past the registry's own destruction.
  [[nodiscard]] base::ScopedClosureRunner Register(
      const std::string& uuid,
      const base::FilePath& folder);

  // Returns null when |uuid| is not registered. The pointee is invalidated by
  // the next Register()/Unregister().
  const base::FilePath* GetFolder(const std::string& uuid) const;

 private:
  void Unregister(const std::string& uuid);

  base::flat_map<std::string, base::FilePath> folders_;
  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<WorkspaceContentRegistry> weak_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_REGISTRY_H_
