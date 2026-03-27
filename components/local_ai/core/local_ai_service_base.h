// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_LOCAL_AI_SERVICE_BASE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_LOCAL_AI_SERVICE_BASE_H_

#include <memory>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "components/keyed_service/core/keyed_service.h"

namespace local_ai {

// Non-template base class for local AI services.
//
// Manages the shared lifecycle: background web contents
// creation, barrier-based readiness (model files + WASM
// factory), and shutdown. Delegates type-specific Mojo
// operations to the subclass via virtual methods.
class LocalAIServiceBase : public KeyedService,
                           public BackgroundWebContents::Delegate {
 public:
  using BackgroundWebContentsFactory =
      base::RepeatingCallback<std::unique_ptr<BackgroundWebContents>(
          BackgroundWebContents::Delegate*)>;

  LocalAIServiceBase(BackgroundWebContentsFactory bwc_factory,
                     bool models_already_available);
  ~LocalAIServiceBase() override;

  LocalAIServiceBase(const LocalAIServiceBase&) = delete;
  LocalAIServiceBase& operator=(const LocalAIServiceBase&) = delete;

 protected:
  // Subclass mojom overrides call these:

  // Lazily creates the background web contents,
  // starting the WASM loading pipeline.
  void MaybeCreateBWC();

  // Returns true if model is ready and factory bound.
  bool ReadyToServe();

  // Model files became available on disk.
  void SignalModelsAvailable();

  // All consumers disconnected — close BWC.
  void NotifyIdle();

  // Subclass RegisterFactory methods call this after
  // binding factory_. Sets disconnect handler + fires
  // one leg of the model load barrier.
  void HandleFactoryRegistered();

  // Resets factory + cancels pending queue. Called by
  // CloseBWC and subclass destructors.
  void CancelPendingAndResetFactory();

  // Queue a consumer. |on_ready| fires when model
  // becomes ready; |on_cancel| fires if factory
  // disconnects or shutdown occurs first.
  void QueueConsumer(base::OnceClosure on_ready, base::OnceClosure on_cancel);

  // -- Subclass must implement --

  // Factory state — wraps subclass's typed factory_.
  virtual bool IsFactoryBound() const = 0;
  virtual void ResetFactory() = 0;
  virtual void SetFactoryDisconnectHandler(base::OnceClosure handler) = 0;

  // Load model files from disk on the thread pool.
  // Call on_done(true) after stashing loaded files
  // internally, or on_done(false) on failure.
  virtual void LoadModelFiles(base::OnceCallback<void(bool)> on_done) = 0;

  // Push loaded model files to factory via Init().
  virtual void InitModelViaFactory(
      base::OnceCallback<void(bool)> on_complete) = 0;

  // Extra shutdown cleanup (clear receivers,
  // invalidate weak ptrs, remove observers).
  virtual void OnShutdownExtra() {}

 private:
  struct PendingConsumer {
    PendingConsumer(base::OnceClosure on_ready, base::OnceClosure on_cancel);
    ~PendingConsumer();
    PendingConsumer(PendingConsumer&&);
    PendingConsumer& operator=(PendingConsumer&&);

    base::OnceClosure on_ready;
    base::OnceClosure on_cancel;
  };

  // BackgroundWebContents::Delegate:
  void OnBackgroundContentsDestroyed(
      BackgroundWebContents::DestroyReason reason) override;

  // KeyedService:
  void Shutdown() override;

  void OnFactoryDisconnected();
  void CloseBWC();
  void InitModelLoadBarrier();
  void OnModelLoadBarrierMet();
  void LoadAndInitModel(base::OnceCallback<void(bool)> on_complete);
  void OnLoadDone(base::OnceCallback<void(bool)> on_complete, bool ok);
  void OnLoadAndInitComplete(bool success);
  void ProcessPendingCallbacks();
  void CancelAllPending();

  std::vector<PendingConsumer> pending_consumers_;
  std::unique_ptr<BackgroundWebContents> bwc_;
  BackgroundWebContentsFactory bwc_factory_;
  // BarrierClosure that fires LoadAndInitModel once
  // both conditions are met: model files on disk +
  // WASM factory registered. Reset on close.
  base::RepeatingClosure model_load_barrier_;
  bool model_files_ready_ = false;
  bool model_ready_ = false;
  base::WeakPtrFactory<LocalAIServiceBase> weak_factory_{this};
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_LOCAL_AI_SERVICE_BASE_H_
