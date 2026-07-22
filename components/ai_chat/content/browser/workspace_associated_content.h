// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_ASSOCIATED_CONTENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_ASSOCIATED_CONTENT_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "content/public/browser/weak_document_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/mojom/content_extraction/ai_page_content.mojom.h"

namespace content {
class BrowserContext;
class RenderFrameHost;
class WebContents;
class NavigationHandle;
}  // namespace content

namespace ai_chat {

// Associated content backed by a hidden, headless chrome-untrusted://workspace
// page. The page holds a FileSystemDirectoryHandle for a user-picked folder
// (delivered by the browser via launchQueue, see M-C) and registers the local
// file tools with Leo via WebMCP (navigator.modelContext). This delegate owns
// the background WebContents, so its lifetime (and the page's) is tied to the
// conversation that owns the delegate. Tool discovery reuses the same
// AIPageContentAgent harvest as tab content (see GetContentTools).
class WorkspaceAssociatedContent : public AssociatedContentDelegate,
                                   public content::WebContentsObserver {
 public:
  WorkspaceAssociatedContent(
      base::FilePath folder_path,
      content::BrowserContext* browser_context,
      base::OnceCallback<void(content::WebContents*)> attach_tab_helpers);
  ~WorkspaceAssociatedContent() override;
  WorkspaceAssociatedContent(const WorkspaceAssociatedContent&) = delete;
  WorkspaceAssociatedContent& operator=(const WorkspaceAssociatedContent&) =
      delete;

  const base::FilePath& folder_path() const { return folder_path_; }

  // AssociatedContentDelegate:
  void GetContent(GetPageContentCallback callback) override;
  void GetContentTools(GetContentToolsCallback callback) override;

  content::WebContents* GetWebContentsForTesting() {
    return web_contents_.get();
  }

 private:
  // content::WebContentsObserver:
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;

  // Grants the workspace origin File System Access read/write permission, mints
  // a directory handle for |folder_path_|, and delivers it to the page's JS via
  // launchQueue. Runs once the page's main frame has loaded.
  void DeliverDirectoryHandle(content::RenderFrameHost* rfh);

  void OnContentToolsFetched(
      GetContentToolsCallback callback,
      content::WeakDocumentPtr rfh,
      mojo::Remote<blink::mojom::AIPageContentAgent> agent,
      blink::mojom::AIPageContentPtr result);

  const base::FilePath folder_path_;
  std::unique_ptr<content::WebContents> web_contents_;

  // True once the workspace page has loaded and its handle has been delivered.
  // Until then GetContentTools reports no tools synchronously, so the manager's
  // add-time probe can't race with (and clobber) the attach we do on load.
  bool page_ready_ = false;

  base::WeakPtrFactory<WorkspaceAssociatedContent> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_ASSOCIATED_CONTENT_H_
