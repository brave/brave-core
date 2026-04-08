// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_FILE_TEXT_EXTRACTOR_BASE_H_
#define BRAVE_BROWSER_AI_CHAT_FILE_TEXT_EXTRACTOR_BASE_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "services/network/public/cpp/web_sandbox_flags.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace ai_chat {

// Base class for extracting text from files by loading them in a hidden
// background WebContents. The renderer acts as a natural sandbox for
// processing untrusted file content.
//
// Subclasses must implement OnDocumentReady() to perform their specific
// text extraction logic and call Finish() when done.
class FileTextExtractorBase : public content::WebContentsDelegate,
                              public content::WebContentsObserver {
 public:
  using ExtractTextCallback =
      base::OnceCallback<void(std::optional<std::string>)>;

  FileTextExtractorBase();
  ~FileTextExtractorBase() override;

  FileTextExtractorBase(const FileTextExtractorBase&) = delete;
  FileTextExtractorBase& operator=(const FileTextExtractorBase&) = delete;

 protected:
  // Called when the document has loaded and is ready for text extraction.
  // Subclasses must implement this and call Finish() with the result.
  virtual void OnDocumentReady() = 0;

  // Returns additional sandbox flags to remove beyond the base set
  // (Scripts, Origin, Navigation). Override to add more, e.g. kPlugins.
  virtual network::mojom::WebSandboxFlags AdditionalUnsandboxFlags() const;

  // Starts loading a file in a hidden WebContents.
  void LoadInWebContents(content::BrowserContext* browser_context,
                         const base::FilePath& file_path);

  // Writes bytes to a temp file, then loads it. |extension| is the file
  // extension to use for MIME type detection (without leading dot).
  void WriteTempFileAndLoad(content::BrowserContext* browser_context,
                            std::vector<uint8_t> file_bytes,
                            const base::FilePath::StringType& extension);

  // Completes extraction. Cleans up and invokes the callback.
  void Finish(std::optional<std::string> result);

  content::WebContents* GetWebContents() { return web_contents_.get(); }

  ExtractTextCallback callback_;

 private:
  // content::WebContentsDelegate:
  bool ShouldSuppressDialogs(content::WebContents* source) override;
  void CanDownload(const GURL& url,
                   const std::string& request_method,
                   base::OnceCallback<void(bool)> callback) override;
  bool IsWebContentsCreationOverridden(
      content::RenderFrameHost* opener,
      content::SiteInstance* source_site_instance,
      content::mojom::WindowContainerType window_container_type,
      const GURL& opener_url,
      const std::string& frame_name,
      const GURL& target_url) override;
  bool CanEnterFullscreenModeForTab(
      content::RenderFrameHost* requesting_frame) override;
  bool CanDragEnter(content::WebContents* source,
                    const content::DropData& data,
                    blink::DragOperationsMask operations_allowed) override;
  void RequestKeyboardLock(content::WebContents* web_contents,
                           bool esc_key_locked) override;

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void PrimaryMainFrameRenderProcessGone(
      base::TerminationStatus status) override;

  void OnTempFileWritten(content::BrowserContext* browser_context,
                         std::optional<base::FilePath> temp_path);
  void OnTimeout();
  void Cleanup();

  std::unique_ptr<content::WebContents> web_contents_;
  base::FilePath temp_file_path_;
  base::OneShotTimer timeout_timer_;
  base::WeakPtrFactory<FileTextExtractorBase> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_FILE_TEXT_EXTRACTOR_BASE_H_
