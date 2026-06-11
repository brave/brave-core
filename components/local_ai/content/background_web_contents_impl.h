// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CONTENT_BACKGROUND_WEB_CONTENTS_IMPL_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CONTENT_BACKGROUND_WEB_CONTENTS_IMPL_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/restricted_web_contents_delegate/restricted_web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "services/network/public/cpp/web_sandbox_flags.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace local_ai {

// Content-layer implementation of BackgroundWebContents. Manages a hidden
// WebContents that runs in the background without any visible UI, modeled
// after chrome/browser/background/background_contents.h.
//
// This class is purely lifecycle — it creates and navigates the
// WebContents, monitors load/crash events, and notifies the delegate.
// It does NOT proxy any Mojo interfaces.
class BackgroundWebContentsImpl : public BackgroundWebContents,
                                  public RestrictedWebContentsDelegate,
                                  public content::WebContentsObserver {
 public:
  using WebContentsCreatedCallback =
      base::OnceCallback<void(content::WebContents*)>;

  // |web_contents_created_callback| is an optional callback invoked
  // immediately after the WebContents is created (e.g. for task manager
  // tagging). It runs before navigation begins.
  //
  // |sandbox_flags| is the starting sandbox of the hidden WebContents.
  // Defaults to "sandbox everything except scripts (needed for JS/WASM)
  // and origin (needed for the Mojo WebUI bridge)". Pages that must be
  // cross-origin isolated (COOP/COEP) need a relaxed value
  // (WebSandboxFlags::kNone), since a sandboxed document can't become
  // cross-origin isolated.
  BackgroundWebContentsImpl(
      content::BrowserContext* browser_context,
      const GURL& url,
      Delegate* delegate,
      WebContentsCreatedCallback web_contents_created_callback =
          WebContentsCreatedCallback(),
      network::mojom::WebSandboxFlags sandbox_flags =
          network::mojom::WebSandboxFlags::kAll &
          ~network::mojom::WebSandboxFlags::kScripts &
          ~network::mojom::WebSandboxFlags::kOrigin);
  ~BackgroundWebContentsImpl() override;

  BackgroundWebContentsImpl(const BackgroundWebContentsImpl&) = delete;
  BackgroundWebContentsImpl& operator=(const BackgroundWebContentsImpl&) =
      delete;

  content::WebContents* web_contents() const { return web_contents_.get(); }

 private:
  // RestrictedWebContentsDelegate:
  void CloseContents(content::WebContents* source) override;

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

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CONTENT_BACKGROUND_WEB_CONTENTS_IMPL_H_
