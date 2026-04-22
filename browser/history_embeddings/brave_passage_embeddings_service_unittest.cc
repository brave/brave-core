// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service.h"

#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/passage_embeddings/public/mojom/passage_embeddings.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace passage_embeddings {

namespace {

constexpr double kTestEmbeddingData[] = {1.0, 2.0, 3.0};

std::vector<double> TestEmbedding() {
  return {std::begin(kTestEmbeddingData), std::end(kTestEmbeddingData)};
}

std::vector<float> TestEmbeddingFloat() {
  return {1.0f, 2.0f, 3.0f};
}

// Fake renderer-side PassageEmbedder. Returns kTestEmbeddingData for
// every call.
class FakeRendererEmbedder : public local_ai::mojom::PassageEmbedder {
 public:
  void GenerateEmbeddings(const std::string& text,
                          GenerateEmbeddingsCallback callback) override {
    embed_count_++;
    std::move(callback).Run(TestEmbedding());
  }

  void AddReceiver(
      mojo::PendingReceiver<local_ai::mojom::PassageEmbedder> receiver) {
    receivers_.Add(this, std::move(receiver));
  }

  int embed_count() const { return embed_count_; }

 private:
  int embed_count_ = 0;
  mojo::ReceiverSet<local_ai::mojom::PassageEmbedder> receivers_;
};

class FakePassageEmbedderFactory
    : public local_ai::mojom::PassageEmbedderFactory {
 public:
  explicit FakePassageEmbedderFactory(FakeRendererEmbedder* worker)
      : worker_(worker) {}

  void Init(local_ai::mojom::ModelFilesPtr model_files,
            InitCallback callback) override {
    init_count_++;
    std::move(callback).Run(init_success_);
  }

  void Bind(mojo::PendingReceiver<local_ai::mojom::PassageEmbedder> receiver)
      override {
    worker_->AddReceiver(std::move(receiver));
  }

  mojo::PendingRemote<local_ai::mojom::PassageEmbedderFactory> BindRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void ResetReceiver() { receiver_.reset(); }

  void set_init_success(bool success) { init_success_ = success; }
  int init_count() const { return init_count_; }

 private:
  raw_ptr<FakeRendererEmbedder> worker_;
  bool init_success_ = true;
  int init_count_ = 0;
  mojo::Receiver<local_ai::mojom::PassageEmbedderFactory> receiver_{this};
};

class FakeBackgroundWebContents : public local_ai::BackgroundWebContents {
 public:
  FakeBackgroundWebContents(Delegate* delegate, base::OnceClosure on_destroyed)
      : delegate_(delegate), on_destroyed_(std::move(on_destroyed)) {}
  ~FakeBackgroundWebContents() override { std::move(on_destroyed_).Run(); }

  void SimulateDestroyed() {
    delegate_->OnBackgroundContentsDestroyed(DestroyReason::kRendererGone);
  }

 private:
  raw_ptr<Delegate> delegate_;
  base::OnceClosure on_destroyed_;
};

}  // namespace

class BravePassageEmbeddingsServiceTest : public testing::Test {
 protected:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(history_embeddings::kHistoryEmbeddings);
    service_ = std::make_unique<BravePassageEmbeddingsService>(
        base::BindRepeating(
            &BravePassageEmbeddingsServiceTest::CreateFakeWebContents,
            base::Unretained(this)),
        local_ai::LocalModelsUpdaterState::GetInstance());
  }

  void TearDown() override {
    service_.reset();
    local_ai::LocalModelsUpdaterState::GetInstance()->SetInstallDir(
        base::FilePath());
  }

  void CreateFakeWebContents(
      local_ai::BackgroundWebContents::Delegate* delegate,
      BravePassageEmbeddingsService::BackgroundWebContentsCreatedCallback
          callback) {
    auto web_contents = std::make_unique<FakeBackgroundWebContents>(
        delegate,
        base::BindOnce(
            [](raw_ptr<FakeBackgroundWebContents>* ref) { *ref = nullptr; },
            &last_created_web_contents_));
    last_created_web_contents_ = web_contents.get();
    std::move(callback).Run(std::move(web_contents));
  }

  void RegisterFactory() {
    mojo::Remote<local_ai::mojom::LocalAIService> local_ai_remote;
    service_->BindLocalAIReceiver(local_ai_remote.BindNewPipeAndPassReceiver());
    local_ai_remote->RegisterPassageEmbedderFactory(fake_factory_.BindRemote());
    local_ai_remote.FlushForTesting();
  }

  void SetUpModelFiles() {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath dir = temp_dir_.GetPath();
    base::FilePath model_dir =
        dir.AppendASCII(local_ai::kEmbeddingGemmaModelDir);
    ASSERT_TRUE(base::CreateDirectory(model_dir));
    ASSERT_TRUE(base::CreateDirectory(
        model_dir.AppendASCII(local_ai::kEmbeddingGemmaDense1Dir)));
    ASSERT_TRUE(base::CreateDirectory(
        model_dir.AppendASCII(local_ai::kEmbeddingGemmaDense2Dir)));
    ASSERT_TRUE(base::WriteFile(
        model_dir.AppendASCII(local_ai::kEmbeddingGemmaModelFile), "w"));
    ASSERT_TRUE(base::WriteFile(
        model_dir.AppendASCII(local_ai::kEmbeddingGemmaDense1Dir)
            .AppendASCII(local_ai::kEmbeddingGemmaDenseModelFile),
        "d1"));
    ASSERT_TRUE(base::WriteFile(
        model_dir.AppendASCII(local_ai::kEmbeddingGemmaDense2Dir)
            .AppendASCII(local_ai::kEmbeddingGemmaDenseModelFile),
        "d2"));
    ASSERT_TRUE(base::WriteFile(
        model_dir.AppendASCII(local_ai::kEmbeddingGemmaTokenizerFile), "t"));
    ASSERT_TRUE(base::WriteFile(
        model_dir.AppendASCII(local_ai::kEmbeddingGemmaConfigFile), "c"));
    local_ai::LocalModelsUpdaterState::GetInstance()->SetInstallDir(dir);
  }

  // Issues BindPassageEmbedder via the direct in-process API and
  // returns a TestFuture for the load result plus a Remote to the batch
  // embedder. Caller drives the system and checks future.Get().
  struct LoadResult {
    base::test::TestFuture<bool> load_success;
    mojo::Remote<mojom::PassageEmbedder> embedder;
  };
  std::unique_ptr<LoadResult> IssueLoad() {
    auto result = std::make_unique<LoadResult>();
    service_->BindPassageEmbedder(result->embedder.BindNewPipeAndPassReceiver(),
                                  result->load_success.GetCallback());
    return result;
  }

  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<BravePassageEmbeddingsService> service_;
  FakeRendererEmbedder fake_worker_;
  FakePassageEmbedderFactory fake_factory_{&fake_worker_};
  raw_ptr<FakeBackgroundWebContents> last_created_web_contents_ = nullptr;
  base::ScopedTempDir temp_dir_;
};

TEST_F(BravePassageEmbeddingsServiceTest, BindCreatesBackgroundContents) {
  auto load = IssueLoad();
  EXPECT_TRUE(last_created_web_contents_);
  EXPECT_FALSE(load->load_success.IsReady());
}

TEST_F(BravePassageEmbeddingsServiceTest, BindWaitsForBothConditions) {
  auto load = IssueLoad();
  ASSERT_TRUE(last_created_web_contents_);

  SetUpModelFiles();
  EXPECT_FALSE(load->load_success.IsReady());
  EXPECT_EQ(0, fake_factory_.init_count());

  RegisterFactory();
  ASSERT_TRUE(
      base::test::RunUntil([&] { return fake_factory_.init_count() > 0; }));
  EXPECT_TRUE(load->load_success.Get());
}

TEST_F(BravePassageEmbeddingsServiceTest, EndToEndBatchGenerateEmbeddings) {
  auto load = IssueLoad();
  SetUpModelFiles();
  RegisterFactory();
  ASSERT_TRUE(load->load_success.Get());

  base::test::TestFuture<std::vector<mojom::PassageEmbeddingsResultPtr>>
      embeddings_future;
  load->embedder->GenerateEmbeddings({"hello", "world"},
                                     mojom::PassagePriority::kPassive,
                                     embeddings_future.GetCallback());

  auto results = embeddings_future.Take();
  ASSERT_EQ(2u, results.size());
  EXPECT_EQ(TestEmbeddingFloat(), results[0]->embeddings);
  EXPECT_EQ(TestEmbeddingFloat(), results[1]->embeddings);
  EXPECT_EQ(2, fake_worker_.embed_count());
}

TEST_F(BravePassageEmbeddingsServiceTest, InitFailureFailsLoad) {
  fake_factory_.set_init_success(false);

  auto load = IssueLoad();
  SetUpModelFiles();
  RegisterFactory();

  ASSERT_TRUE(
      base::test::RunUntil([&] { return fake_factory_.init_count() > 0; }));
  EXPECT_FALSE(load->load_success.Get());
  EXPECT_FALSE(last_created_web_contents_);
}

TEST_F(BravePassageEmbeddingsServiceTest, BackgroundContentsDestroyedCloses) {
  auto load = IssueLoad();
  SetUpModelFiles();
  RegisterFactory();
  ASSERT_TRUE(load->load_success.Get());
  ASSERT_TRUE(last_created_web_contents_);

  last_created_web_contents_->SimulateDestroyed();

  // A new load should recreate background contents.
  auto load2 = IssueLoad();
  EXPECT_TRUE(last_created_web_contents_);
}

TEST_F(BravePassageEmbeddingsServiceTest, BindRegistryRoutesToService) {
  // Manually exercise the static registry: install a bind callback for
  // a fake WebContents pointer, call BindForWebContents, and confirm
  // the callback fires.
  auto* fake_web_contents =
      reinterpret_cast<content::WebContents*>(uintptr_t{0xdeadbeef});
  bool invoked = false;
  BravePassageEmbeddingsService::SetBindCallbackForWebContents(
      fake_web_contents,
      base::BindLambdaForTesting(
          [&](mojo::PendingReceiver<local_ai::mojom::LocalAIService>) {
            invoked = true;
          }));

  mojo::PendingRemote<local_ai::mojom::LocalAIService> dummy_remote;
  BravePassageEmbeddingsService::BindForWebContents(
      fake_web_contents, dummy_remote.InitWithNewPipeAndPassReceiver());
  EXPECT_TRUE(invoked);

  BravePassageEmbeddingsService::RemoveBindCallbackForWebContents(
      fake_web_contents);
  invoked = false;
  mojo::PendingRemote<local_ai::mojom::LocalAIService> dummy_remote2;
  BravePassageEmbeddingsService::BindForWebContents(
      fake_web_contents, dummy_remote2.InitWithNewPipeAndPassReceiver());
  EXPECT_FALSE(invoked);
}

}  // namespace passage_embeddings
