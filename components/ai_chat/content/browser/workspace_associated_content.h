// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_ASSOCIATED_CONTENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_ASSOCIATED_CONTENT_H_

#include <memory>
#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "content/public/browser/weak_document_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/mojom/content_extraction/ai_page_content.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {
class BrowserContext;
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace ai_chat {

// Associated content backed by a hidden, headless
// chrome-untrusted://<uuid>.leo-workspace page, where the uuid is this
// delegate's own, giving each workspace its own origin. The content is
// identified (and stored) by workspace://<uuid>, its |url()|, which is never
// loaded. The page holds a
// FileSystemDirectoryHandle for a user-picked folder (delivered by the browser
// via launchQueue) and registers the local file tools with Leo via WebMCP
// (document.modelContext). This delegate owns the background WebContents, so
// its lifetime (and the page's) is tied to the conversation that owns the
// delegate. Tool discovery reuses the same AIPageContentAgent harvest as tab
// content (see GetContentTools).
class WorkspaceAssociatedContent : public AssociatedContentDelegate,
                                   public content::WebContentsObserver,
                                   public mojom::ContentToolsListener {
 public:
  // Creates a new workspace. A null |folder_path| creates an empty workspace,
  // which is sent no folder, so the page uses its origin private file system
  // (OPFS) as the workspace's folder instead.
  WorkspaceAssociatedContent(
      std::optional<base::FilePath> folder_path,
      content::BrowserContext* browser_context,
      base::OnceCallback<void(content::WebContents*)> attach_tab_helpers);
  ~WorkspaceAssociatedContent() override;
  WorkspaceAssociatedContent(const WorkspaceAssociatedContent&) = delete;
  WorkspaceAssociatedContent& operator=(const WorkspaceAssociatedContent&) =
      delete;

  // Restores a workspace from its workspace://<uuid> URL. Returns nullptr if
  // |url| is not a valid workspace URL.
  static std::unique_ptr<WorkspaceAssociatedContent> CreateFromUrl(
      GURL url,
      content::BrowserContext* browser_context,
      base::OnceCallback<void(content::WebContents*)> attach_tab_helpers);

  // AssociatedContentDelegate:
  void GetContent(GetPageContentCallback callback) override;
  void GetContentTools(GetContentToolsCallback callback) override;
  void OnAssociatedWithConversation() override;
  url::Origin GetOrigin() const override;

  // The URL of the page hosting the workspace's tools:
  // chrome-untrusted://<uuid>.leo-workspace/.
  const GURL& page_url() const { return page_url_; }

  content::WebContents* GetWebContentsForTesting() {
    return web_contents_.get();
  }

 private:
  // Sets up the identity of the workspace with |uuid|, without loading it.
  explicit WorkspaceAssociatedContent(std::string uuid);

  // Creates the hidden WebContents and loads |page_url_| in it.
  void AttachWebContents(
      content::BrowserContext* browser_context,
      base::OnceCallback<void(content::WebContents*)> attach_tab_helpers);

  // content::WebContentsObserver:
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;

  // mojom::ContentToolsListener:
  void OnContentToolsChanged() override;

  // Subscribes to WebMCP tool registration changes from the workspace page.
  void SubscribeToContentToolChanges();

  // Grants the workspace origin File System Access read/write permission, mints
  // a directory handle for |folder_path_|, and delivers it to the page's JS via
  // launchQueue. Runs once the page's main frame has loaded.
  void DeliverDirectoryHandle(content::RenderFrameHost* rfh);

  void OnContentToolsFetched(
      GetContentToolsCallback callback,
      content::WeakDocumentPtr rfh,
      mojo::Remote<blink::mojom::AIPageContentAgent> agent,
      blink::mojom::AIPageContentPtr result);

  // The page hosting the workspace's tools; see page_url().
  const GURL page_url_;

  // The folder path for new workspaces. Empty for empty workspaces, and for
  // restored workspaces, which get their FileSystemDirectoryHandle from
  // IndexedDB instead.
  std::optional<base::FilePath> folder_path_;
  std::unique_ptr<content::WebContents> web_contents_;

  // True once the workspace page has loaded and its handle has been delivered.
  // Until then GetContentTools reports no tools synchronously, so the manager's
  // add-time probe can't race with (and clobber) the attach we do on load.
  bool page_ready_ = false;

  // Mojo bindings for receiving WebMCP tool change notifications.
  mojo::Remote<mojom::PageContentExtractor> content_tools_extractor_;
  mojo::Receiver<mojom::ContentToolsListener> content_tools_listener_{this};

  base::WeakPtrFactory<WorkspaceAssociatedContent> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_ASSOCIATED_CONTENT_H_
