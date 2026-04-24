// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service.h"

#include <utility>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/numerics/safe_math.h"
#include "base/task/thread_pool.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/base/big_buffer.h"

namespace passage_embeddings {

namespace {

// Reads a file directly into BigBuffer storage. For files >64KB
// BigBuffer uses shared memory.
std::optional<mojo_base::BigBuffer> ReadFileToBigBuffer(
    const base::FilePath& path) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    DVLOG(0) << "Failed to open: " << path;
    return std::nullopt;
  }
  int64_t size = file.GetLength();
  if (size <= 0) {
    DVLOG(0) << "Empty or unreadable: " << path;
    return std::nullopt;
  }
  mojo_base::BigBuffer buffer(static_cast<size_t>(size));
  if (!file.ReadAndCheck(0, base::span<uint8_t>(buffer))) {
    DVLOG(0) << "Failed to read: " << path;
    return std::nullopt;
  }
  return buffer;
}

using BindRegistry =
    base::flat_map<content::WebContents*,
                   BravePassageEmbeddingsService::BindCallback>;

BindRegistry& GetBindRegistry() {
  static base::NoDestructor<BindRegistry> registry;
  return *registry;
}

local_ai::mojom::ModelFilesPtr LoadLocalModelFilesFromDisk(
    const base::FilePath& weights_path,
    const base::FilePath& weights_dense1_path,
    const base::FilePath& weights_dense2_path,
    const base::FilePath& tokenizer_path,
    const base::FilePath& config_path) {
  auto weights = ReadFileToBigBuffer(weights_path);
  if (!weights) {
    return nullptr;
  }
  auto weights_dense1 = ReadFileToBigBuffer(weights_dense1_path);
  if (!weights_dense1) {
    return nullptr;
  }
  auto weights_dense2 = ReadFileToBigBuffer(weights_dense2_path);
  if (!weights_dense2) {
    return nullptr;
  }
  auto tokenizer = ReadFileToBigBuffer(tokenizer_path);
  if (!tokenizer) {
    return nullptr;
  }
  auto config = ReadFileToBigBuffer(config_path);
  if (!config) {
    return nullptr;
  }
  auto model_files = local_ai::mojom::ModelFiles::New();
  model_files->weights = std::move(*weights);
  model_files->weights_dense1 = std::move(*weights_dense1);
  model_files->weights_dense2 = std::move(*weights_dense2);
  model_files->tokenizer = std::move(*tokenizer);
  model_files->config = std::move(*config);
  return model_files;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// BraveBatchPassageEmbedder

BraveBatchPassageEmbedder::Batch::Batch() = default;
BraveBatchPassageEmbedder::Batch::~Batch() = default;
BraveBatchPassageEmbedder::Batch::Batch(Batch&&) noexcept = default;
BraveBatchPassageEmbedder::Batch& BraveBatchPassageEmbedder::Batch::operator=(
    Batch&&) noexcept = default;

BraveBatchPassageEmbedder::BraveBatchPassageEmbedder(
    mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
    mojo::Remote<local_ai::mojom::PassageEmbedderFactory> factory,
    local_ai::mojom::ModelFilesPtr model_files,
    base::OnceCallback<void(bool)> load_callback,
    base::OnceClosure on_disconnect)
    : receiver_(this, std::move(receiver)),
      factory_(std::move(factory)),
      load_callback_(std::move(load_callback)),
      on_disconnect_(std::move(on_disconnect)) {
  receiver_.set_disconnect_handler(
      base::BindOnce(&BraveBatchPassageEmbedder::OnDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  factory_.set_disconnect_handler(
      base::BindOnce(&BraveBatchPassageEmbedder::OnDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  factory_->Init(std::move(model_files),
                 base::BindOnce(&BraveBatchPassageEmbedder::OnInitDone,
                                weak_ptr_factory_.GetWeakPtr()));
}

BraveBatchPassageEmbedder::~BraveBatchPassageEmbedder() = default;

void BraveBatchPassageEmbedder::OnInitDone(bool success) {
  if (!success) {
    DVLOG(1) << "Factory Init failed";
    OnDisconnected();
    return;
  }
  factory_->Bind(renderer_embedder_.BindNewPipeAndPassReceiver());
  renderer_embedder_.set_disconnect_handler(
      base::BindOnce(&BraveBatchPassageEmbedder::OnDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  DVLOG(3) << "Factory Init succeeded; signaling load success";
  std::move(load_callback_).Run(true);
}

void BraveBatchPassageEmbedder::GenerateEmbeddings(
    const std::vector<std::string>& passages,
    mojom::PassagePriority priority,
    GenerateEmbeddingsCallback callback) {
  if (passages.empty()) {
    std::move(callback).Run({});
    return;
  }
  DVLOG(3) << "GenerateEmbeddings: queued batch of " << passages.size()
           << " passage(s), priority=" << static_cast<int>(priority);
  Batch batch;
  batch.passages = passages;
  batch.callback = std::move(callback);
  pending_batches_.push_back(std::move(batch));
  if (!current_batch_) {
    ProcessNext();
  }
}

void BraveBatchPassageEmbedder::ProcessNext() {
  CHECK(!current_batch_);
  if (pending_batches_.empty()) {
    return;
  }
  current_batch_.emplace(std::move(pending_batches_.front()));
  pending_batches_.pop_front();

  if (!renderer_embedder_.is_bound()) {
    DVLOG(1) << "Renderer embedder not bound; failing batch of "
             << current_batch_->passages.size() << " passage(s)";
    FailCurrentBatch();
    return;
  }
  DVLOG(3) << "ProcessNext: dispatching passage "
           << current_batch_->next_index + 1 << "/"
           << current_batch_->passages.size();
  const std::string& text =
      current_batch_->passages[current_batch_->next_index];
  renderer_embedder_->GenerateEmbeddings(
      text, base::BindOnce(&BraveBatchPassageEmbedder::OnOneEmbedding,
                           weak_ptr_factory_.GetWeakPtr()));
}

void BraveBatchPassageEmbedder::OnOneEmbedding(
    const std::vector<double>& embedding) {
  if (!current_batch_) {
    return;
  }
  if (embedding.empty()) {
    DVLOG(1) << "Renderer returned empty embedding for passage "
             << current_batch_->next_index + 1 << "/"
             << current_batch_->passages.size() << "; failing batch";
    FailCurrentBatch();
    return;
  }
  auto floats = ToFloatEmbedding(embedding);
  if (!floats) {
    DVLOG(1) << "Embedding value out of float range for passage "
             << current_batch_->next_index + 1 << "/"
             << current_batch_->passages.size() << "; failing batch";
    FailCurrentBatch();
    return;
  }
  auto result = mojom::PassageEmbeddingsResult::New();
  result->embeddings = std::move(*floats);
  current_batch_->results.push_back(std::move(result));
  current_batch_->next_index++;

  if (current_batch_->next_index == current_batch_->passages.size()) {
    DVLOG(3) << "Batch complete: " << current_batch_->results.size()
             << " embedding(s) returned to caller";
    Batch done = std::move(*current_batch_);
    current_batch_.reset();
    std::move(done.callback).Run(std::move(done.results));
    ProcessNext();
    return;
  }
  const std::string& text =
      current_batch_->passages[current_batch_->next_index];
  renderer_embedder_->GenerateEmbeddings(
      text, base::BindOnce(&BraveBatchPassageEmbedder::OnOneEmbedding,
                           weak_ptr_factory_.GetWeakPtr()));
}

void BraveBatchPassageEmbedder::FailCurrentBatch() {
  if (!current_batch_) {
    return;
  }
  Batch failed = std::move(*current_batch_);
  current_batch_.reset();
  std::move(failed.callback).Run({});
  ProcessNext();
}

void BraveBatchPassageEmbedder::OnDisconnected() {
  // If Init hasn't replied yet, fail the outstanding load.
  if (load_callback_) {
    std::move(load_callback_).Run(false);
  }

  std::deque<Batch> to_fail = std::move(pending_batches_);
  if (current_batch_) {
    to_fail.push_front(std::move(*current_batch_));
    current_batch_.reset();
  }
  DVLOG_IF(1, !to_fail.empty())
      << "BatchEmbedder pipe disconnected with " << to_fail.size()
      << " in-flight/queued batch(es); failing all";
  for (auto& batch : to_fail) {
    if (batch.callback) {
      std::move(batch.callback).Run({});
    }
  }
  if (on_disconnect_) {
    std::move(on_disconnect_).Run();
  }
}

// static
std::optional<std::vector<float>> BraveBatchPassageEmbedder::ToFloatEmbedding(
    const std::vector<double>& embedding) {
  std::vector<float> result;
  result.reserve(embedding.size());
  for (double val : embedding) {
    float f;
    if (!base::CheckedNumeric<float>(val).AssignIfValid(&f)) {
      return std::nullopt;
    }
    result.push_back(f);
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// BravePassageEmbeddingsService

BravePassageEmbeddingsService::PendingLoad::PendingLoad() = default;
BravePassageEmbeddingsService::PendingLoad::~PendingLoad() = default;
BravePassageEmbeddingsService::PendingLoad::PendingLoad(
    PendingLoad&&) noexcept = default;
BravePassageEmbeddingsService::PendingLoad&
BravePassageEmbeddingsService::PendingLoad::operator=(PendingLoad&&) noexcept =
    default;

BravePassageEmbeddingsService::BravePassageEmbeddingsService(
    BackgroundWebContentsFactory background_web_contents_factory,
    local_ai::LocalModelsUpdaterState* updater_state)
    : background_web_contents_factory_(
          std::move(background_web_contents_factory)),
      updater_state_(updater_state) {
  CHECK(base::FeatureList::IsEnabled(history_embeddings::kHistoryEmbeddings));
  CHECK(updater_state_);
  // Arm the events FIRST so AddObserver's immediate-notify (when
  // install_dir is already set) has a live component_ready_event_ to
  // signal.
  MaybeWaitForLocalModelFilesReady();
  updater_state_->AddObserver(this);
}

BravePassageEmbeddingsService::~BravePassageEmbeddingsService() {
  updater_state_->RemoveObserver(this);
  CloseBackgroundContents();
}

// static
void BravePassageEmbeddingsService::SetBindCallbackForWebContents(
    content::WebContents* web_contents,
    BindCallback callback) {
  GetBindRegistry()[web_contents] = std::move(callback);
}

// static
void BravePassageEmbeddingsService::RemoveBindCallbackForWebContents(
    content::WebContents* web_contents) {
  GetBindRegistry().erase(web_contents);
}

// static
void BravePassageEmbeddingsService::BindForWebContents(
    content::WebContents* web_contents,
    mojo::PendingReceiver<local_ai::mojom::LocalAIService> receiver) {
  auto& registry = GetBindRegistry();
  auto it = registry.find(web_contents);
  if (it != registry.end()) {
    it->second.Run(std::move(receiver));
  }
}

void BravePassageEmbeddingsService::BindLocalAIReceiver(
    mojo::PendingReceiver<local_ai::mojom::LocalAIService> receiver) {
  local_ai_receivers_.Add(this, std::move(receiver));
}

base::WeakPtr<BravePassageEmbeddingsService>
BravePassageEmbeddingsService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void BravePassageEmbeddingsService::BindPassageEmbedder(
    mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
    base::OnceCallback<void(bool)> callback) {
  // Upstream's controller binds one embedder at a time — the base
  // class gates on `!embedder_remote_`. If we see a second Bind while
  // a load is in flight or an embedder is already active, fail the
  // extra request defensively.
  if (pending_load_ || batch_embedder_) {
    DVLOG(1) << "BindPassageEmbedder called while a load is already in flight "
                "or a BatchEmbedder is already bound; failing";
    std::move(callback).Run(false);
    return;
  }

  MaybeCreateBackgroundContents();

  PendingLoad load;
  load.receiver = std::move(receiver);
  load.callback = std::move(callback);
  pending_load_ = std::move(load);

  MaybeStartLoad();
}

void BravePassageEmbeddingsService::LoadModels(
    mojom::PassageEmbeddingsLoadModelsParamsPtr model_params,
    mojom::PassageEmbedderParamsPtr params,
    mojo::PendingReceiver<mojom::PassageEmbedder> model,
    LoadModelsCallback callback) {
  // Brave does not consume the upstream model file params — we use our
  // own EmbeddingGemma model hosted by a WASM renderer. The params
  // auto-destruct on return (closing any file handles); delegate the
  // receiver to the in-process path.
  BindPassageEmbedder(std::move(model), std::move(callback));
}

void BravePassageEmbeddingsService::RegisterPassageEmbedderFactory(
    mojo::PendingRemote<local_ai::mojom::PassageEmbedderFactory> factory) {
  if (!background_web_contents_) {
    DVLOG(1) << "RegisterPassageEmbedderFactory without background contents";
    return;
  }
  factory_.Bind(std::move(factory));
  factory_.set_disconnect_handler(
      base::BindOnce(&BravePassageEmbeddingsService::OnEarlyFactoryDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  if (factory_registered_event_ && !factory_registered_event_->is_signaled()) {
    factory_registered_event_->Signal();
  }
  MaybeStartLoad();
}

void BravePassageEmbeddingsService::OnLocalModelsReady(
    const base::FilePath& install_dir) {
  if (component_ready_event_ && !component_ready_event_->is_signaled()) {
    component_ready_event_->Signal();
  }
  MaybeStartLoad();
}

void BravePassageEmbeddingsService::OnBackgroundContentsDestroyed(
    local_ai::BackgroundWebContents::DestroyReason reason) {
  DVLOG(1) << "Background contents destroyed unexpectedly, reason="
           << static_cast<int>(reason);
  CloseBackgroundContents();
}

void BravePassageEmbeddingsService::MaybeCreateBackgroundContents() {
  if (background_web_contents_ || background_web_contents_creating_) {
    return;
  }
  background_web_contents_creating_ = true;
  background_web_contents_factory_.Run(
      this, base::BindOnce(
                &BravePassageEmbeddingsService::OnBackgroundContentsCreated,
                weak_ptr_factory_.GetWeakPtr()));
}

void BravePassageEmbeddingsService::OnBackgroundContentsCreated(
    std::unique_ptr<local_ai::BackgroundWebContents> contents) {
  background_web_contents_creating_ = false;
  if (background_web_contents_ || !contents) {
    return;
  }
  background_web_contents_ = std::move(contents);
}

void BravePassageEmbeddingsService::CloseBackgroundContents() {
  DVLOG(3) << "CloseBackgroundContents (pending_load="
           << (pending_load_ ? "set" : "null")
           << " batch_embedder=" << (batch_embedder_ ? "set" : "null")
           << " factory_bound=" << factory_.is_bound()
           << " bg_contents=" << (background_web_contents_ ? "set" : "null")
           << ")";
  ResetEmbedderState();
  background_web_contents_.reset();
  background_web_contents_creating_ = false;
}

void BravePassageEmbeddingsService::MaybeWaitForLocalModelFilesReady() {
  component_ready_event_ = std::make_unique<base::OneShotEvent>();
  factory_registered_event_ = std::make_unique<base::OneShotEvent>();
  if (!updater_state_->GetInstallDir().empty()) {
    component_ready_event_->Signal();
  }
}

void BravePassageEmbeddingsService::MaybeStartLoad() {
  if (!pending_load_ || batch_embedder_) {
    return;
  }
  if (!component_ready_event_ || !component_ready_event_->is_signaled()) {
    return;
  }
  if (!factory_registered_event_ || !factory_registered_event_->is_signaled()) {
    return;
  }
  // The factory_ disconnect handler is owned by the service until the
  // BatchEmbedder takes the remote over; InvalidateWeakPtrs below
  // covers the in-flight ThreadPool reply if ResetEmbedderState races
  // in between.
  LoadLocalModelFiles();
}

void BravePassageEmbeddingsService::LoadLocalModelFiles() {
  base::FilePath weights_path = updater_state_->GetEmbeddingGemmaModel();
  base::FilePath weights_dense1_path =
      updater_state_->GetEmbeddingGemmaDense1();
  base::FilePath weights_dense2_path =
      updater_state_->GetEmbeddingGemmaDense2();
  base::FilePath tokenizer_path = updater_state_->GetEmbeddingGemmaTokenizer();
  base::FilePath config_path = updater_state_->GetEmbeddingGemmaConfig();

  if (updater_state_->GetEmbeddingGemmaModelDir().empty()) {
    ResetEmbedderState();
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&LoadLocalModelFilesFromDisk, weights_path,
                     weights_dense1_path, weights_dense2_path, tokenizer_path,
                     config_path),
      base::BindOnce(&BravePassageEmbeddingsService::OnLocalModelFilesLoaded,
                     load_weak_factory_.GetWeakPtr()));
}

void BravePassageEmbeddingsService::OnLocalModelFilesLoaded(
    local_ai::mojom::ModelFilesPtr model_files) {
  // Any early exit below fails the pending load and re-arms events;
  // ScopedClosureRunner keeps that cleanup in one place.
  base::ScopedClosureRunner reset_on_fail(
      base::BindOnce(&BravePassageEmbeddingsService::ResetEmbedderState,
                     weak_ptr_factory_.GetWeakPtr()));

  if (!model_files) {
    DVLOG(1) << "Model files load failed";
    return;
  }
  if (!factory_.is_bound()) {
    DVLOG(1) << "Factory disconnected while files were loading";
    return;
  }
  if (!pending_load_) {
    DVLOG(1) << "No pending load by the time files loaded";
    return;
  }

  // Success path — hand everything to the BatchEmbedder.
  std::ignore = reset_on_fail.Release();

  PendingLoad load = std::move(*pending_load_);
  pending_load_.reset();
  batch_embedder_ = std::make_unique<BraveBatchPassageEmbedder>(
      std::move(load.receiver), std::move(factory_), std::move(model_files),
      std::move(load.callback),
      base::BindOnce(
          &BravePassageEmbeddingsService::OnBatchEmbedderDisconnected,
          weak_ptr_factory_.GetWeakPtr()));
  DVLOG(3) << "BatchEmbedder created; load flow complete";
}

void BravePassageEmbeddingsService::OnBatchEmbedderDisconnected() {
  DVLOG(3) << "BraveBatchPassageEmbedder disconnected; tearing down";
  // Match the pre-refactor behavior where an Init failure (or any
  // post-init renderer disconnect, e.g. WASM worker crash) tore the
  // background contents down. CloseBackgroundContents handles the
  // ResetEmbedderState work too.
  CloseBackgroundContents();
}

void BravePassageEmbeddingsService::OnEarlyFactoryDisconnected() {
  DVLOG(1) << "Renderer factory pipe disconnected before BatchEmbedder owned "
              "it; resetting";
  ResetEmbedderState();
}

void BravePassageEmbeddingsService::ResetEmbedderState() {
  batch_embedder_.reset();
  factory_.reset();
  if (pending_load_) {
    std::move(pending_load_->callback).Run(false);
    pending_load_.reset();
  }
  // Cancel any in-flight file load reply so it doesn't land after we
  // re-armed the events for the next cycle.
  load_weak_factory_.InvalidateWeakPtrs();
  MaybeWaitForLocalModelFilesReady();
}

}  // namespace passage_embeddings
