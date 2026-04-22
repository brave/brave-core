// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service.h"

#include <utility>

#include "base/barrier_closure.h"
#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/functional/bind.h"
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
    mojo::PendingRemote<local_ai::mojom::PassageEmbedder> renderer_embedder,
    base::OnceClosure on_disconnect)
    : receiver_(this, std::move(receiver)),
      renderer_embedder_(std::move(renderer_embedder)),
      on_disconnect_(std::move(on_disconnect)) {
  receiver_.set_disconnect_handler(
      base::BindOnce(&BraveBatchPassageEmbedder::OnDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  renderer_embedder_.set_disconnect_handler(
      base::BindOnce(&BraveBatchPassageEmbedder::OnDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
}

BraveBatchPassageEmbedder::~BraveBatchPassageEmbedder() = default;

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
  // Arm the barrier FIRST so CountComponentReadyOnce can route through
  // it. AddObserver's immediate-notify (when install_dir is already
  // set) will invoke OnLocalModelsReady synchronously; the flag guard
  // inside CountComponentReadyOnce keeps that from counting twice.
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
  MaybeCreateBackgroundContents();

  PendingLoad load;
  load.receiver = std::move(receiver);
  load.callback = std::move(callback);
  pending_loads_.push_back(std::move(load));

  if (model_ready_ && factory_.is_bound()) {
    FulfillPendingLoads();
  }
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
      base::BindOnce(&BravePassageEmbeddingsService::OnFactoryDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  if (model_load_barrier_) {
    model_load_barrier_.Run();
  }
}

void BravePassageEmbeddingsService::OnLocalModelsReady(
    const base::FilePath& install_dir) {
  CountComponentReadyOnce();
}

void BravePassageEmbeddingsService::OnBackgroundContentsDestroyed(
    local_ai::BackgroundWebContents::DestroyReason reason) {
  DVLOG(1) << "Background contents destroyed unexpectedly, reason="
           << static_cast<int>(reason);
  CloseBackgroundContents();
}

void BravePassageEmbeddingsService::MaybeCreateBackgroundContents() {
  if (background_web_contents_) {
    return;
  }
  background_web_contents_factory_.Run(
      this, base::BindOnce(
                &BravePassageEmbeddingsService::OnBackgroundContentsCreated,
                weak_ptr_factory_.GetWeakPtr()));
}

void BravePassageEmbeddingsService::OnBackgroundContentsCreated(
    std::unique_ptr<local_ai::BackgroundWebContents> contents) {
  if (background_web_contents_ || !contents) {
    return;
  }
  background_web_contents_ = std::move(contents);
}

void BravePassageEmbeddingsService::CloseBackgroundContents() {
  DVLOG(3) << "CloseBackgroundContents (pending_loads=" << pending_loads_.size()
           << " batch_embedder=" << (batch_embedder_ ? "set" : "null")
           << " factory_bound=" << factory_.is_bound()
           << " model_ready=" << model_ready_
           << " bg_contents=" << (background_web_contents_ ? "set" : "null")
           << ")";
  ResetEmbedderState();
  background_web_contents_.reset();
}

void BravePassageEmbeddingsService::MaybeWaitForLocalModelFilesReady() {
  component_ready_counted_ = false;
  model_load_barrier_ = base::BarrierClosure(
      2, base::BindOnce(&BravePassageEmbeddingsService::LoadLocalModelFiles,
                        weak_ptr_factory_.GetWeakPtr()));
  if (!updater_state_->GetInstallDir().empty()) {
    CountComponentReadyOnce();
  }
}

void BravePassageEmbeddingsService::CountComponentReadyOnce() {
  if (component_ready_counted_ || !model_load_barrier_) {
    return;
  }
  component_ready_counted_ = true;
  model_load_barrier_.Run();
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
    OnFactoryInitDone(false);
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
                     weak_ptr_factory_.GetWeakPtr()));
}

void BravePassageEmbeddingsService::OnLocalModelFilesLoaded(
    local_ai::mojom::ModelFilesPtr model_files) {
  if (!model_files || !factory_.is_bound()) {
    OnFactoryInitDone(false);
    return;
  }
  factory_->Init(
      std::move(model_files),
      base::BindOnce(&BravePassageEmbeddingsService::OnFactoryInitDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BravePassageEmbeddingsService::OnFactoryInitDone(bool success) {
  if (!success) {
    DVLOG(1) << "Factory Init failed, failing " << pending_loads_.size()
             << " pending load(s)";
    // CloseBackgroundContents runs FailPendingLoads internally and
    // tears down the rest of the state, matching LocalAIService's
    // old OnPassageEmbedderReady(false) behavior.
    CloseBackgroundContents();
    return;
  }
  DVLOG(3) << "Factory Init succeeded, fulfilling " << pending_loads_.size()
           << " pending load(s)";
  model_ready_ = true;
  FulfillPendingLoads();
}

void BravePassageEmbeddingsService::FulfillPendingLoads() {
  if (!model_ready_ || !factory_.is_bound() || pending_loads_.empty()) {
    return;
  }

  if (!batch_embedder_) {
    // Bind a renderer-side single-passage PassageEmbedder via the
    // factory, then wrap it in a BraveBatchPassageEmbedder that serves
    // the upstream batch receiver.
    mojo::PendingRemote<local_ai::mojom::PassageEmbedder> renderer_remote;
    factory_->Bind(renderer_remote.InitWithNewPipeAndPassReceiver());

    // The first pending load owns the batch receiver; subsequent loads
    // are unexpected since upstream's controller binds one embedder at
    // a time, but handle them defensively below.
    PendingLoad first = std::move(pending_loads_.front());
    pending_loads_.erase(pending_loads_.begin());
    batch_embedder_ = std::make_unique<BraveBatchPassageEmbedder>(
        std::move(first.receiver), std::move(renderer_remote),
        base::BindOnce(
            &BravePassageEmbeddingsService::OnBatchEmbedderDisconnected,
            weak_ptr_factory_.GetWeakPtr()));
    std::move(first.callback).Run(true);
  }

  // Any additional pending loads fail: upstream expects one active
  // embedder remote at a time. The controller gates BindPassageEmbedder
  // behind `!embedder_remote_`, so this branch should only fire if
  // something else races a second binding in.
  DVLOG_IF(1, !pending_loads_.empty())
      << "BraveBatchPassageEmbedder already bound; failing "
      << pending_loads_.size() << " extra pending load(s)";
  for (auto& load : pending_loads_) {
    std::move(load.callback).Run(false);
  }
  pending_loads_.clear();
}

void BravePassageEmbeddingsService::FailPendingLoads() {
  for (auto& load : pending_loads_) {
    std::move(load.callback).Run(false);
  }
  pending_loads_.clear();
}

void BravePassageEmbeddingsService::ResetEmbedderState() {
  batch_embedder_.reset();
  factory_.reset();
  FailPendingLoads();
  model_ready_ = false;
  MaybeWaitForLocalModelFilesReady();
}

void BravePassageEmbeddingsService::OnFactoryDisconnected() {
  DVLOG(1) << "Renderer factory pipe disconnected; embeddings unavailable "
              "until the background WebContents reloads";
  ResetEmbedderState();
}

void BravePassageEmbeddingsService::OnBatchEmbedderDisconnected() {
  DVLOG(3) << "BraveBatchPassageEmbedder disconnected; resetting";
  batch_embedder_.reset();
}

}  // namespace passage_embeddings
