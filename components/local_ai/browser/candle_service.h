// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/browser/local_models_updater.h"
#include "brave/components/local_ai/rust/ffi/candle_embedder.h"
#include "components/keyed_service/core/keyed_service.h"

namespace local_ai {

class CandleService : public KeyedService,
                      public LocalModelsUpdaterState::Observer {
 public:
  using EmbedCallback = base::OnceCallback<void(const std::vector<float>&)>;

  CandleService();
  ~CandleService() override;

  CandleService(const CandleService&) = delete;
  CandleService& operator=(const CandleService&) = delete;

  void Embed(const std::string& text, EmbedCallback callback);

 private:
  // LocalModelsUpdaterState::Observer:
  void OnComponentReady(const base::FilePath& install_dir) override;

  // KeyedService:
  void Shutdown() override;

  void LoadModelFiles();
  void OnModelInitialized(std::unique_ptr<CandleEmbedder> embedder,
                          const std::string& error_message);

  // Model loading state
  int model_load_retry_count_ = 0;
  static constexpr int kMaxModelLoadRetries = 3;

  // Track readiness conditions
  bool component_ready_ = false;
  bool model_initialized_ = false;
  bool initialization_in_progress_ = false;

  // The native Rust embedder
  std::unique_ptr<CandleEmbedder> embedder_;

  // Queue for pending Embed requests while model is initializing
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

  base::WeakPtrFactory<CandleService> weak_ptr_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_
