// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CONTENT_BACKGROUND_WEB_CONTENTS_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CONTENT_BACKGROUND_WEB_CONTENTS_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace local_ai {

// Manages a hidden WebContents that runs in the background without any
// visible UI, modeled after chrome/browser/background/background_contents.h.
//
// The WebContents is created with is_never_composited=true and navigated
// to the provided URL. Lifecycle events are forwarded to the Delegate.
//
// The destructor suppresses delegate callbacks during teardown, so
// the owner can simply destroy this object without additional cleanup.
class BackgroundWebContents : public content::WebContentsDelegate,
                              public content::WebContentsObserver {
 public:
  enum class DestroyReason {
    kClose,         // window.close() or CloseContents
    kInvalidUrl,    // Navigated to an unexpected URL
    kRendererGone,  // Renderer process crashed or was killed
  };

  class Delegate {
   public:
    // Called when DidFinishLoad fires on the background WebContents.
    virtual void OnBackgroundContentsReady() = 0;

    // Called when the background WebContents is destroyed.
    virtual void OnBackgroundContentsDestroyed(DestroyReason reason) = 0;

   protected:
    virtual ~Delegate() = default;
  };

  using WebContentsCreatedCallback =
      base::OnceCallback<void(content::WebContents*)>;

  // |web_contents_created_callback| is an optional callback invoked
  // immediately after the WebContents is created (e.g. for task manager
  // tagging). It runs before navigation begins.
  BackgroundWebContents(
      content::BrowserContext* browser_context,
      const GURL& url,
      Delegate* delegate,
      WebContentsCreatedCallback web_contents_created_callback =
          WebContentsCreatedCallback());
  ~BackgroundWebContents() override;

  BackgroundWebContents(const BackgroundWebContents&) = delete;
  BackgroundWebContents& operator=(const BackgroundWebContents&) = delete;

  content::WebContents* web_contents() const { return web_contents_.get(); }

 private:
  // content::WebContentsDelegate:
  void CloseContents(content::WebContents* source) override;
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
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void PrimaryMainFrameRenderProcessGone(
      base::TerminationStatus status) override;

  void NotifyDestroyed(DestroyReason reason);

  raw_ptr<Delegate> delegate_;
  GURL expected_url_;
  std::unique_ptr<content::WebContents> web_contents_;
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CONTENT_BACKGROUND_WEB_CONTENTS_H_
