// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "components/optimization_guide/proto/passage_embeddings_model_metadata.pb.h"
#include "components/passage_embeddings/core/passage_embeddings_service_launcher.h"
#include "content/public/browser/service_process_host.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace passage_embeddings {

namespace {

// The LiteRT model files ship in the shared EmbeddingGemma component, under a
// litert/ subdir of its model dir.
constexpr char kLitertModelSubdir[] = "litert";
constexpr char kLitertModelName[] =
    "embeddinggemma-300M_seq512_mixed-precision.tflite";
constexpr char kSentencePieceModelName[] = "sentencepiece.model";
// Matches the seq512 model's input window. The embedder itself derives the
// actual window from the model's input tensor, so this only feeds the metadata
// the controller reports; keep it in sync with the model above.
constexpr uint32_t kLitertInputWindowSize = 512;
constexpr int kLitertOutputSize = 768;
constexpr double kLitertScoreThreshold = 0.45;
constexpr int kLitertModelVersion = 2;

// Launches the sandboxed Passage Embeddings utility process, whose LoadModels
// is chromium_src-overridden to run EmbeddingGemma on LiteRT's CompiledModel
// inside that process.
class LitertServiceLauncher : public PassageEmbeddingsServiceLauncher {
 public:
  static PassageEmbeddingsServiceLauncher& Create() {
    static base::NoDestructor<LitertServiceLauncher> launcher;
    return *launcher;
  }

  void LaunchService(mojo::PendingReceiver<mojom::PassageEmbeddingsService>
                         receiver) override {
    content::ServiceProcessHost::Launch<mojom::PassageEmbeddingsService>(
        std::move(receiver), content::ServiceProcessHost::Options()
                                 .WithDisplayName("Passage Embeddings Service")
                                 .Pass());
  }
  void OnServiceDisconnected(bool is_idle) override {}
  bool AllowedToLaunch() const override { return true; }
};

}  // namespace

// static
BravePassageEmbeddingsServiceController*
BravePassageEmbeddingsServiceController::Get() {
  static base::NoDestructor<BravePassageEmbeddingsServiceController> instance;
  return instance.get();
}

BravePassageEmbeddingsServiceController::
    BravePassageEmbeddingsServiceController()
    : PassageEmbeddingsServiceController(LitertServiceLauncher::Create()) {
  // Report the LiteRT model's metadata; the model file paths are resolved from
  // the component in OnLocalModelsReady once it is installed.
  optimization_guide::proto::PassageEmbeddingsModelMetadata metadata;
  metadata.set_input_window_size(kLitertInputWindowSize);
  metadata.set_output_size(kLitertOutputSize);
  metadata.set_score_threshold(kLitertScoreThreshold);
  model_metadata_ = std::move(metadata);
  model_version_ = kLitertModelVersion;

  // AddObserver re-fires OnLocalModelsReady synchronously if the component is
  // already installed; our handler resolves the model paths and notifies
  // observer_list_ via EmbedderMetadataUpdated.
  updater_state_observation_.Observe(
      local_ai::LocalModelsUpdaterState::GetInstance());
}

BravePassageEmbeddingsServiceController::
    ~BravePassageEmbeddingsServiceController() = default;

bool BravePassageEmbeddingsServiceController::MaybeUpdateModelInfo(
    base::optional_ref<const optimization_guide::ModelInfo> model_info) {
  // No-op: we don't consume optimization_guide's tflite model. See header.
  return false;
}

bool BravePassageEmbeddingsServiceController::IsModelAvailable() {
  // The model paths are resolved from the component in OnLocalModelsReady.
  return !embeddings_model_path_.empty();
}

void BravePassageEmbeddingsServiceController::OnLocalModelsReady(
    const base::FilePath& install_dir) {
  if (install_dir.empty()) {
    // Component uninstall (or test reset) cleared the dir; drop the resolved
    // model paths so IsModelAvailable() reports unavailable.
    embeddings_model_path_ = base::FilePath();
    sp_model_path_ = base::FilePath();
    return;
  }
  // The LiteRT .tflite + SentencePiece model ship in the EmbeddingGemma
  // component under a litert/ subdir. They may be absent (an older component,
  // or the download withheld), so confirm both are on disk before reporting the
  // model available -- the base-class LoadModels flow opens them
  // unconditionally and would crash serializing a missing (null) file.
  const base::FilePath litert_dir =
      local_ai::LocalModelsUpdaterState::GetInstance()
          ->GetEmbeddingGemmaModelDir()
          .AppendASCII(kLitertModelSubdir);
  const base::FilePath embeddings_model_path =
      litert_dir.AppendASCII(kLitertModelName);
  const base::FilePath sp_model_path =
      litert_dir.AppendASCII(kSentencePieceModelName);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(
          [](const base::FilePath& embeddings, const base::FilePath& sp) {
            return base::PathExists(embeddings) && base::PathExists(sp);
          },
          embeddings_model_path, sp_model_path),
      base::BindOnce(
          &BravePassageEmbeddingsServiceController::OnLitertModelChecked,
          base::Unretained(this), embeddings_model_path, sp_model_path));
}

void BravePassageEmbeddingsServiceController::OnLitertModelChecked(
    const base::FilePath& embeddings_model_path,
    const base::FilePath& sp_model_path,
    bool models_exist) {
  if (!models_exist) {
    VLOG(1) << "LiteRT model files missing under "
            << embeddings_model_path.DirName()
            << "; passage embeddings disabled until the component ships them";
    return;
  }
  embeddings_model_path_ = embeddings_model_path;
  sp_model_path_ = sp_model_path;
  // SchedulingEmbedder is the only observer and SubmitWorkToEmbedder()
  // short-circuits if work is already in flight, so re-firing on repeated
  // component-updater notifications is harmless.
  observer_list_.Notify(&EmbedderMetadataObserver::EmbedderMetadataUpdated,
                        GetEmbedderMetadata());
}

EmbedderMetadata
BravePassageEmbeddingsServiceController::GetEmbedderMetadata() {
  // A distinct model_version from the previous WASM embedder so SqlDatabase
  // re-embeds stored history rather than mixing vector spaces.
  return EmbedderMetadata(kLitertModelVersion,
                          /*output_size=*/kLitertOutputSize,
                          /*search_score_threshold=*/kLitertScoreThreshold);
}

}  // namespace passage_embeddings
