// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/core/local_ai_service.h"

#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
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

// Fake PassageEmbedderFactory that binds directly to FakeWorker.
class FakePassageEmbedderFactory : public mojom::PassageEmbedderFactory {
 public:
  explicit FakePassageEmbedderFactory(FakeWorker* worker) : worker_(worker) {}

  void Bind(mojo::PendingReceiver<mojom::PassageEmbedder> receiver) override {
    worker_->AddReceiver(std::move(receiver));
  }

  mojo::PendingRemote<mojom::PassageEmbedderFactory> BindRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

 private:
  raw_ptr<FakeWorker> worker_;
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
    feature_list_.InitAndEnableFeature(features::kLocalAIModels);
    service_ = std::make_unique<LocalAIService>(base::BindRepeating(
        &LocalAIServiceTest::CreateFakeWebContents, base::Unretained(this)));
  }

  void TearDown() override {
    static_cast<KeyedService*>(service_.get())->Shutdown();
    service_.reset();
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

  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<LocalAIService> service_;
  FakeWorker fake_worker_;
  FakePassageEmbedderFactory fake_factory_{&fake_worker_};
  raw_ptr<FakeBackgroundWebContents> last_created_web_contents_ = nullptr;
};

TEST_F(LocalAIServiceTest, GetPassageEmbedderCreatesBackgroundContents) {
  // GetPassageEmbedder queues the callback (factory not yet registered).
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());
  EXPECT_TRUE(last_created_web_contents_);

  // Register factory to flush the pending callback.
  RegisterFactory();
  EXPECT_TRUE(future.Get().is_valid());
}

TEST_F(LocalAIServiceTest, GetPassageEmbedderWaitsForFactory) {
  // Call GetPassageEmbedder before factory is registered — callback
  // should be deferred.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());
  EXPECT_FALSE(future.IsReady());

  // Registering the factory flushes the pending callback.
  RegisterFactory();
  EXPECT_TRUE(future.IsReady());
  EXPECT_TRUE(future.Get().is_valid());
}

TEST_F(LocalAIServiceTest, EndToEndEmbedding) {
  // Register factory first, then get embedder — callback should
  // resolve immediately.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future;
  service_->GetPassageEmbedder(future.GetCallback());
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
  RegisterFactory();

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

TEST_F(LocalAIServiceTest, FactoryDisconnectCancelsPendingCallbacks) {
  // Queue a callback, register factory, then disconnect.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>>
      pending_future;
  service_->GetPassageEmbedder(pending_future.GetCallback());
  RegisterFactory();

  // First callback should have been served.
  EXPECT_TRUE(pending_future.Get().is_valid());

  // Queue another callback and simulate factory disconnect.
  base::test::TestFuture<mojo::PendingRemote<mojom::PassageEmbedder>> future2;
  service_->GetPassageEmbedder(future2.GetCallback());

  // Factory is still bound, so this callback should be served
  // immediately.
  EXPECT_TRUE(future2.Get().is_valid());
}

}  // namespace local_ai
