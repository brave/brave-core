// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CONTENT_LOCAL_AI_SERVICE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CONTENT_LOCAL_AI_SERVICE_H_

#include <memory>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace local_ai {

class LocalAIService;

// Helper class to observe WebContents for WASM page load events.
// This is separated from LocalAIService so the service can be shared
// with platforms that don't have content::WebContentsObserver.
class WasmWebContentsObserver : public content::WebContentsObserver {
 public:
  explicit WasmWebContentsObserver(LocalAIService* service,
                                   content::WebContents* web_contents);
  ~WasmWebContentsObserver() override;

  WasmWebContentsObserver(const WasmWebContentsObserver&) = delete;
  WasmWebContentsObserver& operator=(const WasmWebContentsObserver&) = delete;

 private:
  // content::WebContentsObserver:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  raw_ptr<LocalAIService> service_;
};

// LocalAIService provides on-device machine learning capabilities, currently
// using the Candle ML framework for execution via WebAssembly.
//
// This service manages:
// - A hidden WebContents that loads and runs the WASM-based ML model
// - Communication between the browser process and the WASM renderer via Mojo
// - Request queueing while the model initializes
// - Automatic cleanup after idle timeout to free memory
//
// The service is currently implemented using Candle (see candle_embedding_gemma
// resources), but the API is framework-agnostic to allow future flexibility.
class LocalAIService : public KeyedService, public mojom::LocalAIService {
 public:
  explicit LocalAIService(content::BrowserContext* browser_context);
  ~LocalAIService() override;

  LocalAIService(const LocalAIService&) = delete;
  LocalAIService& operator=(const LocalAIService&) = delete;

  mojo::PendingRemote<mojom::LocalAIService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::LocalAIService> receiver);

  void BindEmbeddingGemma(
      mojo::PendingRemote<mojom::EmbeddingGemmaInterface>) override;

  void Embed(const std::string& text, EmbedCallback callback) override;

  // Called by WasmWebContentsObserver when the WASM page finishes
  // loading
  void OnWasmPageLoaded();

 private:
  friend class WasmWebContentsObserver;

  // KeyedService:
  void Shutdown() override;

  void EnsureWasmWebContents();
  void CloseWasmWebContents();
  void StartIdleTimer();
  void StopIdleTimer();

  raw_ptr<content::BrowserContext> browser_context_ = nullptr;

  // The single WebContents that loads the WASM and maintains the
  // model
  std::unique_ptr<content::WebContents> wasm_web_contents_;

  // Observer for WASM WebContents load events
  std::unique_ptr<WasmWebContentsObserver> wasm_web_contents_observer_;

  mojo::ReceiverSet<mojom::LocalAIService> receivers_;

  // Single embedder remote (shared by all callers)
  mojo::Remote<mojom::EmbeddingGemmaInterface> embedding_gemma_remote_;

  // Track readiness conditions
  bool wasm_page_loaded_ = false;
  bool embedding_ready_ = false;

  // Holds an Embed() call that arrived before the WASM model was ready.
  // Requests are drained in FIFO order once the model is initialized.
  struct PendingEmbedRequest {
    PendingEmbedRequest();
    PendingEmbedRequest(std::string text, EmbedCallback callback);
    ~PendingEmbedRequest();
    PendingEmbedRequest(PendingEmbedRequest&&);
    PendingEmbedRequest& operator=(PendingEmbedRequest&&);

    std::string text;
    EmbedCallback callback;
  };
  std::vector<PendingEmbedRequest> pending_embed_requests_;

  void ProcessPendingEmbedRequests();

  // Idle timer to close WebContents when not in use
  base::OneShotTimer idle_timer_;

  base::WeakPtrFactory<LocalAIService> weak_ptr_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CONTENT_LOCAL_AI_SERVICE_H_
