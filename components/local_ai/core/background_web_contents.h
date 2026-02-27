// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_BACKGROUND_WEB_CONTENTS_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_BACKGROUND_WEB_CONTENTS_H_

#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace local_ai {

// Abstract interface for a background web environment that runs local AI
// model workers. The core/ layer talks only through this interface -- it
// never sees WebContents, BrowserContext, or any content-layer type.
//
// Desktop: implemented by BackgroundWebContentsImpl (content/).
// iOS: would be implemented by a WKWebView-based equivalent.
class BackgroundWebContents {
 public:
  enum class DestroyReason {
    kClose,         // window.close() or CloseContents
    kInvalidUrl,    // Navigated to an unexpected URL
    kRendererGone,  // Renderer process crashed or was killed
  };

  class Delegate {
   public:
    // Called when the background environment has finished loading and
    // is ready to receive mojo connections.
    virtual void OnBackgroundContentsReady() = 0;

    // Called when the background environment is destroyed unexpectedly
    // (renderer crash, window.close, invalid URL). The BackgroundWebContents
    // instance is invalid after this call.
    virtual void OnBackgroundContentsDestroyed(DestroyReason reason) = 0;

   protected:
    virtual ~Delegate() = default;
  };

  virtual ~BackgroundWebContents() = default;

  // Stores the renderer's PassageEmbedder implementation and binds any
  // pending consumer receivers.
  virtual void SetWorkerRemote(
      mojo::PendingRemote<mojom::PassageEmbedder> remote) = 0;

  // Creates a new Mojo pipe and returns the consumer end. If the worker
  // is already connected, the receiver is bound immediately; otherwise
  // it is held until SetWorkerRemote() is called.
  virtual mojo::PendingRemote<mojom::PassageEmbedder>
  BindNewPassageEmbedder() = 0;
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_BACKGROUND_WEB_CONTENTS_H_
