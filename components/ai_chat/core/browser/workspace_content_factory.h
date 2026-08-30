// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_CONTENT_FACTORY_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_CONTENT_FACTORY_H_

#include <memory>
#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/functional/callback_forward.h"

namespace ai_chat {

class AssociatedContentDelegate;

// Creates the associated content backing a Leo workspace. That needs a
// WebContents, so this interface lets a conversation restore a workspace
// without depending on the layer that has one.
class WorkspaceContentFactory {
 public:
  using CreateWorkspaceContentCallback =
      base::OnceCallback<void(std::unique_ptr<AssociatedContentDelegate>)>;

  virtual ~WorkspaceContentFactory() = default;

  // Creates content exposing Leo's file tools over |folder_path|. Pass the
  // persisted |uuid| when restoring a workspace so it keeps its identity,
  // nullopt for a new one. Calls back with nullptr if |folder_path| is no
  // longer a usable directory - checking that is why this is asynchronous.
  virtual void CreateWorkspaceContent(
      const base::FilePath& folder_path,
      std::optional<std::string> uuid,
      CreateWorkspaceContentCallback callback) = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_CONTENT_FACTORY_H_
