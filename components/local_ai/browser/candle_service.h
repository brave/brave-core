// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/browser/local_models_updater.h"
#include "brave/components/local_ai/common/candle.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/common/messaging/web_message_port.h"
#include "third_party/blink/public/common/service_worker/service_worker_status_code.h"

namespace content {
class BrowserContext;
class ServiceWorkerContext;
}  // namespace content

namespace local_ai {

// CandleService runs WASM-based embedding models using a Service Worker
// to minimize memory usage. It communicates with the Service Worker via
// MessagePort for bidirectional message passing.
class CandleService : public KeyedService,
                      public mojom::CandleService,
                      public LocalModelsUpdaterState::Observer,
                      public blink::WebMessagePort::MessageReceiver {
 public:
  explicit CandleService(content::BrowserContext* browser_context);
  ~CandleService() override;

  CandleService(const CandleService&) = delete;
  CandleService& operator=(const CandleService&) = delete;

  void BindReceiver(mojo::PendingReceiver<mojom::CandleService> receiver);

  void BindEmbeddingGemma(
      mojo::PendingRemote<mojom::EmbeddingGemmaInterface>) override;

  void Embed(const std::string& text, EmbedCallback callback) override;

 private:
  // LocalModelsUpdaterState::Observer:
  void OnLocalModelsReady(const base::FilePath& install_dir) override;

  // KeyedService:
  void Shutdown() override;

  // blink::WebMessagePort::MessageReceiver:
  bool OnMessage(blink::WebMessagePort::Message message) override;
  void OnPipeError() override;

  void EnsureServiceWorker();
  void OnServiceWorkerRegistered(blink::ServiceWorkerStatusCode status);
  void StartServiceWorkerWithPort();
  void OnServiceWorkerStartResult(bool success);
  void OnServiceWorkerStarted(int64_t version_id,
                              int process_id,
                              int thread_id);
  void SendMessageToServiceWorker(blink::WebMessagePort::Message message);
  void LoadModelFiles();
  void OnModelFilesLoaded(mojom::ModelFilesPtr model_files);
  void ProcessPendingEmbedRequests();

  // Struct to hold loaded file data
  struct LoadedFiles {
    LoadedFiles();
    ~LoadedFiles();
    LoadedFiles(LoadedFiles&&);
    LoadedFiles& operator=(LoadedFiles&&);

    std::vector<uint8_t> weights;
    std::vector<uint8_t> dense1;
    std::vector<uint8_t> dense2;
    std::vector<uint8_t> tokenizer;
    std::vector<uint8_t> config;
  };

  static std::optional<LoadedFiles> LoadFilesOnBackgroundThread(
      const base::FilePath& weights_path,
      const base::FilePath& dense1_path,
      const base::FilePath& dense2_path,
      const base::FilePath& tokenizer_path,
      const base::FilePath& config_path);
  void OnFilesLoaded(std::optional<LoadedFiles> files);
  void SendInitWithDataMessage();

  raw_ptr<content::BrowserContext> browser_context_ = nullptr;
  raw_ptr<content::ServiceWorkerContext> service_worker_context_ = nullptr;

  mojo::ReceiverSet<mojom::CandleService> receivers_;

  // MessagePort for communication with the Service Worker
  blink::WebMessagePort message_port_;

  // Track Service Worker state
  bool service_worker_registered_ = false;
  bool service_worker_started_ = false;
  bool port_connected_ = false;
  bool model_initialized_ = false;
  bool component_ready_ = false;
  int64_t service_worker_version_id_ = -1;

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

  // Map message IDs to callbacks for async responses
  int next_message_id_ = 1;
  std::map<int, mojom::CandleService::EmbedCallback> pending_callbacks_;

  // Pending files to send via MessagePort (base64 encoded)
  std::optional<LoadedFiles> pending_model_files_;
  scoped_refptr<base::RefCountedMemory> pending_wasm_bytes_;

  base::WeakPtrFactory<CandleService> weak_ptr_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_BROWSER_CANDLE_SERVICE_H_
