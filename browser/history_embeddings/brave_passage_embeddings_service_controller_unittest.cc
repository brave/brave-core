// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"

#include <optional>
#include <string_view>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/raw_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "components/optimization_guide/core/optimization_guide_proto_util.h"
#include "components/optimization_guide/proto/models.pb.h"
#include "components/optimization_guide/proto/passage_embeddings_model_metadata.pb.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace passage_embeddings {

namespace {

// Deliberately unlike the shipping model's values, so a hard-coded number
// cannot pass these tests.
constexpr uint32_t kInputWindowSize = 256u;
constexpr size_t kOutputSize = 64u;
constexpr double kScoreThreshold = 0.5;

class TestMetadataObserver : public EmbedderMetadataObserver {
 public:
  void EmbedderMetadataUpdated(EmbedderMetadata metadata) override {
    metadata_ = metadata;
  }

  const std::optional<EmbedderMetadata>& metadata() const { return metadata_; }

 private:
  std::optional<EmbedderMetadata> metadata_;
};

}  // namespace

class BravePassageEmbeddingsServiceControllerTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    controller_->AddObserver(&observer_);
  }

  void TearDown() override {
    controller_->RemoveObserver(&observer_);
    // The updater state is a process-wide singleton that ignores an install dir
    // it already holds, so clear it for whichever test runs next.
    local_ai::LocalModelsUpdaterState::GetInstance()->SetInstallDir(
        base::FilePath());
  }

 protected:
  // Creates the model files the EmbeddingGemma component ships, under an
  // install dir of its own so each call looks like a distinct component version
  // to the updater. Returns the install dir.
  base::FilePath CreateComponentDir(std::string_view name) {
    const base::FilePath install_dir = temp_dir_.GetPath().AppendASCII(name);
    const base::FilePath model_dir = ModelDir(install_dir);
    CHECK(base::CreateDirectory(model_dir));
    CHECK(base::WriteFile(model_dir.AppendASCII("model.tflite"), "tflite"));
    CHECK(base::WriteFile(model_dir.AppendASCII("sentencepiece.model"), "sp"));
    return install_dir;
  }

  // Adds the metadata file, as generated for the component in
  // brave/leo-local-models. A component dir without it stands in for one that
  // predates the file.
  void WriteModelInfo(const base::FilePath& install_dir, int64_t version) {
    optimization_guide::proto::PassageEmbeddingsModelMetadata metadata;
    metadata.set_input_window_size(kInputWindowSize);
    metadata.set_output_size(kOutputSize);
    metadata.set_score_threshold(kScoreThreshold);

    optimization_guide::proto::ModelInfo model_info;
    model_info.set_optimization_target(
        optimization_guide::proto::OPTIMIZATION_TARGET_PASSAGE_EMBEDDER);
    model_info.set_version(version);
    *model_info.mutable_model_metadata() =
        optimization_guide::AnyWrapProto(metadata);
    // A basename: the loader resolves it against the model dir.
    model_info.add_additional_files()->set_file_path("sentencepiece.model");
    CHECK(base::WriteFile(ModelDir(install_dir).AppendASCII("model-info.pb"),
                          model_info.SerializeAsString()));
  }

  void InstallComponent(const base::FilePath& install_dir) {
    local_ai::LocalModelsUpdaterState::GetInstance()->SetInstallDir(
        install_dir);
  }

 private:
  static base::FilePath ModelDir(const base::FilePath& install_dir) {
    return install_dir.AppendASCII(local_ai::kEmbeddingGemmaModelDir)
        .AppendASCII("litert");
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  TestMetadataObserver observer_;
  // Process-wide singleton, so every test picks a distinct model version: the
  // base class treats a repeat of the version it already holds as a no-op.
  raw_ptr<BravePassageEmbeddingsServiceController> controller_ =
      BravePassageEmbeddingsServiceController::Get();
};

// The version and the embedder metadata come from the component's
// model-info.pb, not from the browser binary.
TEST_F(BravePassageEmbeddingsServiceControllerTest,
       ReadsMetadataFromModelInfo) {
  const base::FilePath component = CreateComponentDir("with-model-info");
  WriteModelInfo(component, /*version=*/11);
  InstallComponent(component);

  ASSERT_TRUE(
      base::test::RunUntil([&] { return observer_.metadata().has_value(); }));
  EXPECT_EQ(11, observer_.metadata()->model_version);
  EXPECT_EQ(kOutputSize, observer_.metadata()->output_size);
  EXPECT_EQ(kScoreThreshold, observer_.metadata()->search_score_threshold);
  EXPECT_TRUE(controller_->IsModelAvailable());
}

// A component that ships no model info publishes no model, and takes any model
// published earlier with it: those paths point into the dir it replaces.
TEST_F(BravePassageEmbeddingsServiceControllerTest,
       ComponentWithoutModelInfoClearsModel) {
  const base::FilePath with_model_info = CreateComponentDir("with-model-info");
  WriteModelInfo(with_model_info, /*version=*/12);
  InstallComponent(with_model_info);
  ASSERT_TRUE(
      base::test::RunUntil([&] { return observer_.metadata().has_value(); }));
  ASSERT_TRUE(controller_->IsModelAvailable());

  InstallComponent(CreateComponentDir("no-model-info"));
  EXPECT_TRUE(
      base::test::RunUntil([&] { return !controller_->IsModelAvailable(); }));
}

// Losing the component takes the model with it: the master switch turning off
// unregisters the component and removes its files, so the paths published from
// its install dir are gone.
TEST_F(BravePassageEmbeddingsServiceControllerTest,
       ClearedInstallDirClearsModel) {
  const base::FilePath component = CreateComponentDir("with-model-info");
  WriteModelInfo(component, /*version=*/13);
  InstallComponent(component);
  ASSERT_TRUE(
      base::test::RunUntil([&] { return observer_.metadata().has_value(); }));
  ASSERT_TRUE(controller_->IsModelAvailable());

  InstallComponent(base::FilePath());
  EXPECT_FALSE(controller_->IsModelAvailable());
}

// A load still in flight when the models go away must not publish what it
// read: those paths are about to be removed with the component.
TEST_F(BravePassageEmbeddingsServiceControllerTest,
       StaleLoadDoesNotRepublishModel) {
  const base::FilePath component = CreateComponentDir("going-away");
  WriteModelInfo(component, /*version=*/14);
  InstallComponent(component);
  // Take the models away without waiting, so the load is still in flight.
  InstallComponent(base::FilePath());

  // Let the load finish - that posts its reply - then drain the reply behind a
  // sentinel queued after it.
  base::ThreadPoolInstance::Get()->FlushForTesting();
  bool drained = false;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindLambdaForTesting([&] { drained = true; }));
  ASSERT_TRUE(base::test::RunUntil([&] { return drained; }));

  EXPECT_FALSE(controller_->IsModelAvailable());
}

}  // namespace passage_embeddings
