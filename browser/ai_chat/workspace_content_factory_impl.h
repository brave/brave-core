// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_WORKSPACE_CONTENT_FACTORY_IMPL_H_
#define BRAVE_BROWSER_AI_CHAT_WORKSPACE_CONTENT_FACTORY_IMPL_H_

#include <memory>
#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/workspace_content_factory.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace ai_chat {

// Builds WorkspaceAssociatedContent, which needs a BrowserContext to host its
// page. Installed on the AIChatService so conversations can restore workspaces
// without depending on this layer.
class WorkspaceContentFactoryImpl : public WorkspaceContentFactory {
 public:
  // |browser_context| must outlive this factory, which its owning
  // AIChatService guarantees.
  explicit WorkspaceContentFactoryImpl(
      content::BrowserContext* browser_context);
  ~WorkspaceContentFactoryImpl() override;
  WorkspaceContentFactoryImpl(const WorkspaceContentFactoryImpl&) = delete;
  WorkspaceContentFactoryImpl& operator=(const WorkspaceContentFactoryImpl&) =
      delete;

  // WorkspaceContentFactory:
  void CreateWorkspaceContent(const base::FilePath& folder_path,
                              std::optional<std::string> uuid,
                              CreateWorkspaceContentCallback callback) override;

 private:
  void OnFolderChecked(base::FilePath folder_path,
                       std::optional<std::string> uuid,
                       CreateWorkspaceContentCallback callback,
                       bool folder_exists);

  const raw_ptr<content::BrowserContext> browser_context_;

  base::WeakPtrFactory<WorkspaceContentFactoryImpl> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_WORKSPACE_CONTENT_FACTORY_IMPL_H_
