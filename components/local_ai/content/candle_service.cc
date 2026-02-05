// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/content/candle_service.h"

#include "base/logging.h"
#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"

namespace local_ai {

CandleService::PendingEmbedRequest::PendingEmbedRequest() = default;
CandleService::PendingEmbedRequest::PendingEmbedRequest(std::string text,
                                                        EmbedCallback callback)
    : text(std::move(text)), callback(std::move(callback)) {}
CandleService::PendingEmbedRequest::~PendingEmbedRequest() = default;
CandleService::PendingEmbedRequest::PendingEmbedRequest(PendingEmbedRequest&&) =
    default;
CandleService::PendingEmbedRequest&
CandleService::PendingEmbedRequest::operator=(PendingEmbedRequest&&) = default;

// WasmWebContentsObserver implementation
WasmWebContentsObserver::WasmWebContentsObserver(
    CandleService* service,
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), service_(service) {}

WasmWebContentsObserver::~WasmWebContentsObserver() = default;

void WasmWebContentsObserver::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  DVLOG(3) << "WasmWebContentsObserver: WASM page loaded: " << validated_url;
  if (service_) {
    service_->OnWasmPageLoaded();
  }
}

namespace {
// Idle timeout before closing the WASM WebContents to free memory
constexpr base::TimeDelta kIdleTimeout = base::Minutes(1);
}  // namespace

// CandleService implementation
CandleService::CandleService(content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  DVLOG(3) << "CandleService created for browser context";

  if (!browser_context_) {
    DVLOG(0) << "CandleService: No browser context available";
    return;
  }
}

CandleService::~CandleService() {
  CloseWasmWebContents();
}

void CandleService::BindReceiver(
    mojo::PendingReceiver<mojom::CandleService> receiver) {
  receivers_.Add(this, std::move(receiver));
  DVLOG(3) << "BindReceiver called";
}

void CandleService::BindEmbeddingGemma(
    mojo::PendingRemote<mojom::EmbeddingGemmaInterface> pending_remote) {
  // Bind the single embedder remote from our WASM WebContents
  if (embedding_gemma_remote_.is_bound()) {
    DVLOG(1) << "EmbeddingGemma already bound, resetting";
    embedding_gemma_remote_.reset();
    embedding_ready_ = false;
  }
  embedding_gemma_remote_.Bind(std::move(pending_remote));

  // Set up disconnect handler - this handles renderer crashes,
  // manual kills, etc.
  embedding_gemma_remote_.set_disconnect_handler(base::BindOnce(
      [](CandleService* service) {
        DVLOG(1) << "EmbeddingGemma remote disconnected "
                    "(renderer crash or manual kill)";
        // Clear pending requests on disconnect
        for (auto& request : service->pending_embed_requests_) {
          std::move(request.callback).Run({});
        }
        service->pending_embed_requests_.clear();
        // Close WebContents and reset state so next Embed() will
        // reinitialize
        service->CloseWasmWebContents();
      },
      base::Unretained(this)));

  DVLOG(3) << "BindEmbeddingGemma: Bound embedder remote";

  // Mark as ready and process pending requests
  embedding_ready_ = true;
  ProcessPendingEmbedRequests();
}

void CandleService::Embed(const std::string& text, EmbedCallback callback) {
  // Stop idle timer since we have activity
  StopIdleTimer();

  // Ensure WebContents exists (may have been closed due to idle)
  EnsureWasmWebContents();

  // If remote is not ready yet, queue the request
  if (!embedding_ready_) {
    DVLOG(3) << "Embedding not ready yet, queuing embed request";
    pending_embed_requests_.emplace_back(text, std::move(callback));
    return;
  }

  embedding_gemma_remote_->Embed(text, std::move(callback));

  // Start idle timer after processing the request
  StartIdleTimer();
}

void CandleService::OnWasmPageLoaded() {
  DVLOG(3) << "CandleService: WASM page loaded";
  wasm_page_loaded_ = true;
}

void CandleService::Shutdown() {
  DVLOG(3) << "CandleService: Shutting down";

  // Clear any pending requests
  for (auto& request : pending_embed_requests_) {
    std::move(request.callback).Run({});
  }
  pending_embed_requests_.clear();

  CloseWasmWebContents();
}

void CandleService::ProcessPendingEmbedRequests() {
  if (!embedding_ready_ || !embedding_gemma_remote_) {
    return;
  }

  DVLOG(3) << "Processing " << pending_embed_requests_.size()
           << " pending embed requests";

  // Process all queued requests
  for (auto& request : pending_embed_requests_) {
    embedding_gemma_remote_->Embed(request.text, std::move(request.callback));
  }
  pending_embed_requests_.clear();

  // Start idle timer after processing requests
  StartIdleTimer();
}

void CandleService::EnsureWasmWebContents() {
  if (wasm_web_contents_) {
    return;  // Already created
  }

  DVLOG(3) << "CandleService: Creating WASM WebContents";

  // Create a hidden WebContents to load the WASM
  content::WebContents::CreateParams create_params(browser_context_);
  create_params.is_never_composited = true;
  wasm_web_contents_ = content::WebContents::Create(create_params);

  // Create observer for the WebContents to track page load
  wasm_web_contents_observer_ =
      std::make_unique<WasmWebContentsObserver>(this, wasm_web_contents_.get());

  // Navigate to the WASM page - this will trigger
  // BindEmbeddingGemma automatically
  GURL wasm_url(kUntrustedCandleEmbeddingGemmaWasmURL);
  DVLOG(3) << "CandleService: Loading WASM from " << wasm_url;
  wasm_web_contents_->GetController().LoadURL(wasm_url, content::Referrer(),
                                              ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                              std::string());
}

void CandleService::CloseWasmWebContents() {
  DVLOG(3) << "CandleService: Closing WASM WebContents "
              "to free memory";

  idle_timer_.Stop();
  wasm_web_contents_observer_.reset();
  embedding_gemma_remote_.reset();

  if (wasm_web_contents_) {
    wasm_web_contents_->Close();
    wasm_web_contents_.reset();
  }

  // Reset state so we can reinitialize later
  wasm_page_loaded_ = false;
  embedding_ready_ = false;
}

void CandleService::StartIdleTimer() {
  idle_timer_.Start(FROM_HERE, kIdleTimeout,
                    base::BindOnce(&CandleService::CloseWasmWebContents,
                                   weak_ptr_factory_.GetWeakPtr()));
}

void CandleService::StopIdleTimer() {
  idle_timer_.Stop();
}

}  // namespace local_ai
