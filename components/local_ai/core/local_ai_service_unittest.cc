// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/core/local_ai_service.h"

#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

namespace {

constexpr double kTestEmbeddingData[] = {1.0, 2.0, 3.0};

std::vector<double> TestEmbedding() {
  return {std::begin(kTestEmbeddingData), std::end(kTestEmbeddingData)};
}

// Fake PassageEmbedder implementation (acts as the renderer worker).
// Supports multiple consumers via ReceiverSet.
class FakeWorker : public mojom::PassageEmbedder {
 public:
  void GenerateEmbeddings(const std::string& text,
                          GenerateEmbeddingsCallback callback) override {
    embed_count_++;
    std::move(callback).Run(TestEmbedding());
  }

  void AddReceiver(mojo::PendingReceiver<mojom::PassageEmbedder> receiver) {
    receivers_.Add(this, std::move(receiver));
  }

  void ClearReceivers() { receivers_.Clear(); }

  int embed_count() const { return embed_count_; }

 private:
  int embed_count_ = 0;
  mojo::ReceiverSet<mojom::PassageEmbedder> receivers_;
};

// Fake PassageEmbedderFactory that binds directly to FakeWorker
// and tracks Init() calls for model loading tests.
class FakePassageEmbedderFactory : public mojom::PassageEmbedderFactory {
 public:
  explicit FakePassageEmbedderFactory(FakeWorker* worker) : worker_(worker) {}

  void Init(mojom::ModelFilesPtr model_files, InitCallback callback) override {
    init_count_++;
    std::move(callback).Run(init_success_);
  }

  void Bind(mojo::PendingReceiver<mojom::PassageEmbedder> receiver) override {
    worker_->AddReceiver(std::move(receiver));
  }

  mojo::PendingRemote<mojom::PassageEmbedderFactory> BindRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void ResetReceiver() { receiver_.reset(); }

  void set_init_success(bool success) { init_success_ = success; }
  int init_count() const { return init_count_; }

 private:
  raw_ptr<FakeWorker> worker_;
  bool init_success_ = true;
  int init_count_ = 0;
  mojo::Receiver<mojom::PassageEmbedderFactory> receiver_{this};
};

// Pure lifecycle fake — no Mojo proxy code.
// On destruction, clears the FakeWorker's receivers to simulate the
// renderer dying when the real BackgroundWebContents is destroyed.
class FakeBackgroundWebContents : public BackgroundWebContents {
 public:
  FakeBackgroundWebContents(Delegate* delegate,
                            FakeWorker* worker,
                            base::OnceClosure on_destroyed)
      : delegate_(delegate),
        worker_(worker),
        on_destroyed_(std::move(on_destroyed)) {}
  ~FakeBackgroundWebContents() override {
    worker_->ClearReceivers();
    std::move(on_destroyed_).Run();
  }

  void SimulateDestroyed() {
    delegate_->OnBackgroundContentsDestroyed(DestroyReason::kRendererGone);
  }

 private:
  raw_ptr<Delegate> delegate_;
  raw_ptr<FakeWorker> worker_;
  base::OnceClosure on_destroyed_;
};

}  // namespace

class LocalAIServiceTest : public testing::Test {
 protected:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(history_embeddings::kHistoryEmbeddings);
    service_ = std::make_unique<LocalAIService>(
        base::BindRepeating(&LocalAIServiceTest::CreateFakeWebContents,
                            base::Unretained(this)),
        LocalModelsUpdaterState::GetInstance());
  }

  void TearDown() override {
    static_cast<KeyedService*>(service_.get())->Shutdown();
    service_.reset();
    // Clear the singleton state for test isolation.
    LocalModelsUpdaterState::GetInstance()->SetInstallDir(base::FilePath());
  }

  std::unique_ptr<BackgroundWebContents> CreateFakeWebContents(
      BackgroundWebContents::Delegate* delegate) {
    auto web_contents = std::make_unique<FakeBackgroundWebContents>(
        delegate, &fake_worker_,
        base::BindOnce(
            [](raw_ptr<FakeBackgroundWebContents>* ref) { *ref = nullptr; },
            &last_created_web_contents_));
    last_created_web_contents_ = web_contents.get();
    return web_contents;
  }

  // Helper: register the factory with the service.
  void RegisterFactory() {
    service_->RegisterPassageEmbedderFactory(fake_factory_.BindRemote());
  }

  // Helper: get a PassageEmbedder remote from the service.
  mojo::Remote<mojom::PassageEmbedder> GetEmbedder() {
    base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
    service_->GetPassageEmbedder(future.GetCallback());
    mojo::Remote<mojom::PassageEmbedder> remote;
    remote.Bind(future.Take());
    return remote;
  }

  // Set up dummy model files on disk and notify the updater state.
  void SetUpModelFiles() {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath dir = temp_dir_.GetPath();
    base::FilePath model_dir = dir.AppendASCII(kEmbeddingGemmaModelDir);
    ASSERT_TRUE(base::CreateDirectory(model_dir));
    ASSERT_TRUE(
        base::CreateDirectory(model_dir.AppendASCII(kEmbeddingGemmaDense1Dir)));
    ASSERT_TRUE(
        base::CreateDirectory(model_dir.AppendASCII(kEmbeddingGemmaDense2Dir)));

    // Create dummy model files matching expected paths.
    ASSERT_TRUE(
        base::WriteFile(model_dir.AppendASCII(kEmbeddingGemmaModelFile), "w"));
    ASSERT_TRUE(base::WriteFile(model_dir.AppendASCII(kEmbeddingGemmaDense1Dir)
                                    .AppendASCII(kEmbeddingGemmaDenseModelFile),
                                "d1"));
    ASSERT_TRUE(base::WriteFile(model_dir.AppendASCII(kEmbeddingGemmaDense2Dir)
                                    .AppendASCII(kEmbeddingGemmaDenseModelFile),
                                "d2"));
    ASSERT_TRUE(base::WriteFile(
        model_dir.AppendASCII(kEmbeddingGemmaTokenizerFile), "t"));
    ASSERT_TRUE(
        base::WriteFile(model_dir.AppendASCII(kEmbeddingGemmaConfigFile), "c"));

    // This fires OnLocalModelsReady on the service.
    LocalModelsUpdaterState::GetInstance()->SetInstallDir(dir);
  }

  // Simulate both readiness conditions being met:
  // 1. Component ready (OnLocalModelsReady via SetInstallDir)
  // 2. Factory registered (RegisterPassageEmbedderFactory — also sets
  //    wasm_page_loaded_)
  // After these, TryLoadModel() will trigger Init() on the factory.
  void MakeFullyReady() {
    // Request an embedder (creates background contents, queues callback).
    base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
    service_->GetPassageEmbedder(future.GetCallback());

    // 1. Component ready (sets up files + notifies).
    SetUpModelFiles();
    // 2. Register the factory (sets wasm_page_loaded_, triggers
    //    TryLoadModel which starts model loading).
    RegisterFactory();

    // Wait for model loading (thread-pool hop + factory Init round-trip)
    // which also flushes the pending embedder callback.
    ASSERT_TRUE(
        base::test::RunUntil([&] { return fake_factory_.init_count() > 0; }));
    ASSERT_TRUE(future.Get().is_valid());
  }

  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<LocalAIService> service_;
  FakeWorker fake_worker_;
  FakePassageEmbedderFactory fake_factory_{&fake_worker_};
  raw_ptr<FakeBackgroundWebContents> last_created_web_contents_ = nullptr;
  base::ScopedTempDir temp_dir_;
};

TEST_F(LocalAIServiceTest, GetPassageEmbedderCreatesBackgroundContents) {
  // GetPassageEmbedder queues the callback (factory not yet registered).
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());
  EXPECT_TRUE(last_created_web_contents_);

  // Pending callback is only served once model is fully loaded.
  SetUpModelFiles();
  RegisterFactory();
  EXPECT_TRUE(future.Get().is_valid());
}

TEST_F(LocalAIServiceTest, GetPassageEmbedderWaitsForModelReady) {
  // Call GetPassageEmbedder before anything is ready — callback
  // should be deferred.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());
  EXPECT_FALSE(future.IsReady());

  // Model files alone aren't enough (no factory yet).
  SetUpModelFiles();
  EXPECT_FALSE(future.IsReady());

  // Once factory registers and model loads, callback is served.
  RegisterFactory();
  EXPECT_TRUE(future.Get().is_valid());
}

TEST_F(LocalAIServiceTest, EndToEndEmbedding) {
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());
  SetUpModelFiles();
  RegisterFactory();

  mojo::Remote<mojom::PassageEmbedder> embedder;
  embedder.Bind(future.Take());

  base::test::TestFuture<const std::vector<double>&> future1;
  base::test::TestFuture<const std::vector<double>&> future2;
  embedder->GenerateEmbeddings("hello", future1.GetCallback());
  embedder->GenerateEmbeddings("world", future2.GetCallback());

  EXPECT_EQ(TestEmbedding(), future1.Get());
  EXPECT_EQ(TestEmbedding(), future2.Get());
  EXPECT_EQ(2, fake_worker_.embed_count());
}

TEST_F(LocalAIServiceTest, MultipleConsumersEachGetResults) {
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> f1;
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> f2;
  service_->GetPassageEmbedder(f1.GetCallback());
  service_->GetPassageEmbedder(f2.GetCallback());
  SetUpModelFiles();
  RegisterFactory();

  mojo::Remote<mojom::PassageEmbedder> embedder1;
  embedder1.Bind(f1.Take());
  mojo::Remote<mojom::PassageEmbedder> embedder2;
  embedder2.Bind(f2.Take());

  base::test::TestFuture<const std::vector<double>&> future1;
  base::test::TestFuture<const std::vector<double>&> future2;
  embedder1->GenerateEmbeddings("consumer 1", future1.GetCallback());
  embedder2->GenerateEmbeddings("consumer 2", future2.GetCallback());

  EXPECT_EQ(TestEmbedding(), future1.Get());
  EXPECT_EQ(TestEmbedding(), future2.Get());
  EXPECT_EQ(2, fake_worker_.embed_count());
}

TEST_F(LocalAIServiceTest, ShutdownDisconnectsConsumers) {
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());
  SetUpModelFiles();
  RegisterFactory();

  mojo::Remote<mojom::PassageEmbedder> embedder;
  embedder.Bind(future.Take());

  // Ensure the factory's Bind message is delivered by doing a
  // round-trip on the embedder pipe. This guarantees the receiver is
  // bound in FakeWorker before Shutdown tears things down.
  base::test::TestFuture<const std::vector<double>&> ping;
  embedder->GenerateEmbeddings("ping", ping.GetCallback());
  EXPECT_EQ(TestEmbedding(), ping.Get());

  static_cast<KeyedService*>(service_.get())->Shutdown();

  embedder.FlushForTesting();
  EXPECT_FALSE(embedder.is_connected());
}

TEST_F(LocalAIServiceTest, ShutdownCancelsPendingCallbacks) {
  // Queue a callback without registering the factory.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());

  static_cast<KeyedService*>(service_.get())->Shutdown();

  // Pending callback should receive an invalid remote.
  EXPECT_FALSE(future.Get().is_valid());
}

TEST_F(LocalAIServiceTest, ReinitializesAfterDestroyed) {
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());
  SetUpModelFiles();
  RegisterFactory();
  ASSERT_TRUE(future.Get().is_valid());

  ASSERT_TRUE(last_created_web_contents_);
  last_created_web_contents_->SimulateDestroyed();

  // Getting a new embedder should create new background contents.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future2;
  service_->GetPassageEmbedder(future2.GetCallback());
  EXPECT_TRUE(last_created_web_contents_);
}

TEST_F(LocalAIServiceTest, DoubleShutdownIsIdempotent) {
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());

  auto* keyed_service = static_cast<KeyedService*>(service_.get());
  keyed_service->Shutdown();
  keyed_service->Shutdown();
}

TEST_F(LocalAIServiceTest, NotifyPassageEmbedderIdleClosesBackgroundContents) {
  // Create background contents and register factory.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());
  SetUpModelFiles();
  RegisterFactory();
  ASSERT_TRUE(base::test::RunUntil([&] { return future.IsReady(); }));
  ASSERT_TRUE(future.Get().is_valid());
  ASSERT_TRUE(last_created_web_contents_);

  // Worker reports idle — should close background contents.
  service_->NotifyPassageEmbedderIdle();
  EXPECT_FALSE(last_created_web_contents_);

  // Getting a new embedder should recreate background contents.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future2;
  service_->GetPassageEmbedder(future2.GetCallback());
  EXPECT_TRUE(last_created_web_contents_);
}

TEST_F(LocalAIServiceTest, GetPassageEmbedderAfterModelReady) {
  // Queue a callback, load model, register factory.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>>
      pending_future;
  service_->GetPassageEmbedder(pending_future.GetCallback());
  SetUpModelFiles();
  RegisterFactory();

  // First callback should have been served after model loaded.
  EXPECT_TRUE(pending_future.Get().is_valid());

  // Queue another callback — model is initialized and factory bound,
  // so this callback should be served immediately.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future2;
  service_->GetPassageEmbedder(future2.GetCallback());
  EXPECT_TRUE(future2.Get().is_valid());
}

// Model loading tests

TEST_F(LocalAIServiceTest, TryLoadModelWaitsForBothConditions) {
  // Request embedder to create background contents.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());

  // Only component ready - not enough (no factory).
  SetUpModelFiles();
  EXPECT_EQ(0, fake_factory_.init_count());

  // Both conditions met - now Init() should be triggered on the factory.
  RegisterFactory();

  // Wait for thread-pool hop (LoadModelFiles) and mojo round-trip
  // (factory_->Init) to complete.
  ASSERT_TRUE(
      base::test::RunUntil([&] { return fake_factory_.init_count() > 0; }));
  EXPECT_EQ(1, fake_factory_.init_count());
}

TEST_F(LocalAIServiceTest, TryLoadModelDoesNotLoadIfAlreadyInitialized) {
  MakeFullyReady();
  EXPECT_EQ(1, fake_factory_.init_count());
}

TEST_F(LocalAIServiceTest, ComponentReadyBeforeFactoryTriggersLoad) {
  // Request embedder to create background contents.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());

  // Component ready first, then factory registers.
  SetUpModelFiles();
  RegisterFactory();

  // Wait for thread-pool hop (LoadModelFiles) and mojo round-trip
  // (factory_->Init) to complete.
  ASSERT_TRUE(
      base::test::RunUntil([&] { return fake_factory_.init_count() > 0; }));
  EXPECT_EQ(1, fake_factory_.init_count());
}

TEST_F(LocalAIServiceTest, RegisterFactoryAloneIsNotReady) {
  // Request embedder to create background contents.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());
  RegisterFactory();

  // Factory registered but no model files — Init should not be called.
  EXPECT_EQ(0, fake_factory_.init_count());
}

TEST_F(LocalAIServiceTest, InitFailureCancelsPendingCallbacks) {
  fake_factory_.set_init_success(false);

  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> f1;
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> f2;
  service_->GetPassageEmbedder(f1.GetCallback());
  service_->GetPassageEmbedder(f2.GetCallback());

  SetUpModelFiles();
  RegisterFactory();

  // Init() fails — all pending consumers should get null remotes.
  EXPECT_FALSE(f1.Get().is_valid());
  EXPECT_FALSE(f2.Get().is_valid());
}

TEST_F(LocalAIServiceTest, InitFailureAllowsRetryOnNewRequest) {
  fake_factory_.set_init_success(false);

  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> f1;
  service_->GetPassageEmbedder(f1.GetCallback());
  SetUpModelFiles();
  RegisterFactory();

  // First attempt fails.
  EXPECT_FALSE(f1.Get().is_valid());
  EXPECT_EQ(1, fake_factory_.init_count());

  // Background contents should have been torn down.
  EXPECT_FALSE(last_created_web_contents_);

  // A new request recreates background contents, and once the factory
  // re-registers the whole flow retries. Reset the receiver first —
  // in production each WASM page creates a new factory instance; here
  // we reuse the fake so need to unbind before rebinding.
  fake_factory_.set_init_success(true);
  fake_factory_.ResetReceiver();
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> f2;
  service_->GetPassageEmbedder(f2.GetCallback());
  EXPECT_TRUE(last_created_web_contents_);

  // Simulate the new WASM page registering its factory.
  RegisterFactory();
  EXPECT_TRUE(f2.Get().is_valid());
  EXPECT_EQ(2, fake_factory_.init_count());
}

TEST_F(LocalAIServiceTest, MissingModelFilesCancelsPendingCallbacks) {
  // Set an install dir that exists but has no model files inside it.
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath dir = temp_dir_.GetPath();
  ASSERT_TRUE(base::CreateDirectory(dir.AppendASCII(kEmbeddingGemmaModelDir)));
  LocalModelsUpdaterState::GetInstance()->SetInstallDir(dir);

  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());
  RegisterFactory();

  // Disk read fails — consumer should get a null remote.
  EXPECT_FALSE(future.Get().is_valid());
}

TEST_F(LocalAIServiceTest, LargeModelFilesUseSharedMemory) {
  // Create model files larger than BigBuffer's inline threshold (64KB)
  // to exercise the shared memory path.
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath dir = temp_dir_.GetPath();
  base::FilePath model_dir = dir.AppendASCII(kEmbeddingGemmaModelDir);
  ASSERT_TRUE(base::CreateDirectory(model_dir));
  ASSERT_TRUE(
      base::CreateDirectory(model_dir.AppendASCII(kEmbeddingGemmaDense1Dir)));
  ASSERT_TRUE(
      base::CreateDirectory(model_dir.AppendASCII(kEmbeddingGemmaDense2Dir)));

  const size_t kLargeSize = mojo_base::BigBuffer::kMaxInlineBytes + 1;
  std::string large_data(kLargeSize, 'x');

  ASSERT_TRUE(base::WriteFile(model_dir.AppendASCII(kEmbeddingGemmaModelFile),
                              large_data));
  ASSERT_TRUE(base::WriteFile(model_dir.AppendASCII(kEmbeddingGemmaDense1Dir)
                                  .AppendASCII(kEmbeddingGemmaDenseModelFile),
                              large_data));
  ASSERT_TRUE(base::WriteFile(model_dir.AppendASCII(kEmbeddingGemmaDense2Dir)
                                  .AppendASCII(kEmbeddingGemmaDenseModelFile),
                              large_data));
  ASSERT_TRUE(base::WriteFile(
      model_dir.AppendASCII(kEmbeddingGemmaTokenizerFile), large_data));
  ASSERT_TRUE(
      base::WriteFile(model_dir.AppendASCII(kEmbeddingGemmaConfigFile), "c"));

  LocalModelsUpdaterState::GetInstance()->SetInstallDir(dir);

  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());
  RegisterFactory();

  EXPECT_TRUE(future.Get().is_valid());
  EXPECT_EQ(1, fake_factory_.init_count());
}

}  // namespace local_ai
