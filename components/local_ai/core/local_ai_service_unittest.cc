// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/core/local_ai_service.h"

#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/run_until.h"
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
class FakeWorker : public mojom::PassageEmbedder {
 public:
  void GenerateEmbeddings(const std::string& text,
                          GenerateEmbeddingsCallback callback) override {
    embed_count_++;
    std::move(callback).Run(TestEmbedding());
  }

  mojo::PendingRemote<mojom::PassageEmbedder> BindNewPipeAndPassRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void Reset() { receiver_.reset(); }

  int embed_count() const { return embed_count_; }

 private:
  int embed_count_ = 0;
  mojo::Receiver<mojom::PassageEmbedder> receiver_{this};
};

// Fake BackgroundWebContents that implements SetWorkerRemote and
// BindNewPassageEmbedder with a simple proxy PassageEmbedder.
class FakeBackgroundWebContents : public BackgroundWebContents,
                                  public mojom::PassageEmbedder {
 public:
  FakeBackgroundWebContents(Delegate* delegate, base::OnceClosure on_destroyed)
      : delegate_(delegate), on_destroyed_(std::move(on_destroyed)) {}
  ~FakeBackgroundWebContents() override { std::move(on_destroyed_).Run(); }

  void SimulateReady() { delegate_->OnBackgroundContentsReady(); }
  void SimulateDestroyed() {
    delegate_->OnBackgroundContentsDestroyed(DestroyReason::kRendererGone);
  }

  // BackgroundWebContents:
  void SetWorkerRemote(
      mojo::PendingRemote<mojom::PassageEmbedder> remote) override {
    worker_remote_.Bind(std::move(remote));
    for (auto& receiver : pending_receivers_) {
      receivers_.Add(this, std::move(receiver));
    }
    pending_receivers_.clear();
  }

  mojo::PendingRemote<mojom::PassageEmbedder> BindNewPassageEmbedder()
      override {
    mojo::PendingRemote<mojom::PassageEmbedder> remote;
    auto receiver = remote.InitWithNewPipeAndPassReceiver();
    if (worker_remote_.is_bound()) {
      receivers_.Add(this, std::move(receiver));
    } else {
      pending_receivers_.push_back(std::move(receiver));
    }
    return remote;
  }

  // mojom::PassageEmbedder:
  void GenerateEmbeddings(const std::string& text,
                          GenerateEmbeddingsCallback callback) override {
    if (worker_remote_.is_bound()) {
      worker_remote_->GenerateEmbeddings(text, std::move(callback));
    } else {
      std::move(callback).Run({});
    }
  }

 private:
  raw_ptr<Delegate> delegate_;
  base::OnceClosure on_destroyed_;
  mojo::Remote<mojom::PassageEmbedder> worker_remote_;
  mojo::ReceiverSet<mojom::PassageEmbedder> receivers_;
  std::vector<mojo::PendingReceiver<mojom::PassageEmbedder>> pending_receivers_;
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
        delegate,
        base::BindOnce(
            [](raw_ptr<FakeBackgroundWebContents>* ref) { *ref = nullptr; },
            &last_created_web_contents_));
    last_created_web_contents_ = web_contents.get();
    return web_contents;
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
  raw_ptr<FakeBackgroundWebContents> last_created_web_contents_ = nullptr;
};

TEST_F(LocalAIServiceTest, GetPassageEmbedderCreatesBackgroundContents) {
  auto embedder = GetEmbedder();
  EXPECT_TRUE(last_created_web_contents_);
}

TEST_F(LocalAIServiceTest, RegisterPassageEmbedderForwardsToWebContents) {
  auto embedder = GetEmbedder();
  ASSERT_TRUE(last_created_web_contents_);

  service_->RegisterPassageEmbedder(fake_worker_.BindNewPipeAndPassRemote());

  base::test::TestFuture<const std::vector<double>&> future;
  embedder->GenerateEmbeddings("test", future.GetCallback());
  EXPECT_EQ(TestEmbedding(), future.Get());
}

TEST_F(LocalAIServiceTest, EndToEndEmbedding) {
  // Get embedder, register worker, call GenerateEmbeddings.
  auto embedder = GetEmbedder();
  service_->RegisterPassageEmbedder(fake_worker_.BindNewPipeAndPassRemote());

  base::test::TestFuture<const std::vector<double>&> future1;
  base::test::TestFuture<const std::vector<double>&> future2;
  embedder->GenerateEmbeddings("hello", future1.GetCallback());
  embedder->GenerateEmbeddings("world", future2.GetCallback());

  EXPECT_EQ(TestEmbedding(), future1.Get());
  EXPECT_EQ(TestEmbedding(), future2.Get());
  EXPECT_EQ(2, fake_worker_.embed_count());
}

TEST_F(LocalAIServiceTest, MultipleConsumersEachGetResults) {
  auto embedder1 = GetEmbedder();
  auto embedder2 = GetEmbedder();
  service_->RegisterPassageEmbedder(fake_worker_.BindNewPipeAndPassRemote());

  base::test::TestFuture<const std::vector<double>&> future1;
  base::test::TestFuture<const std::vector<double>&> future2;
  embedder1->GenerateEmbeddings("consumer 1", future1.GetCallback());
  embedder2->GenerateEmbeddings("consumer 2", future2.GetCallback());

  EXPECT_EQ(TestEmbedding(), future1.Get());
  EXPECT_EQ(TestEmbedding(), future2.Get());
  EXPECT_EQ(2, fake_worker_.embed_count());
}

TEST_F(LocalAIServiceTest, ShutdownDisconnectsConsumers) {
  auto embedder = GetEmbedder();
  service_->RegisterPassageEmbedder(fake_worker_.BindNewPipeAndPassRemote());

  static_cast<KeyedService*>(service_.get())->Shutdown();

  // After shutdown, background contents is destroyed, disconnecting
  // the embedder remote.
  EXPECT_TRUE(base::test::RunUntil([&] { return !embedder.is_connected(); }));
}

TEST_F(LocalAIServiceTest, ReinitializesAfterDestroyed) {
  auto embedder1 = GetEmbedder();
  service_->RegisterPassageEmbedder(fake_worker_.BindNewPipeAndPassRemote());

  ASSERT_TRUE(last_created_web_contents_);
  last_created_web_contents_->SimulateDestroyed();

  // Getting a new embedder should create new background contents.
  auto embedder2 = GetEmbedder();
  EXPECT_TRUE(last_created_web_contents_);
}

TEST_F(LocalAIServiceTest, DoubleShutdownIsIdempotent) {
  auto embedder = GetEmbedder();

  auto* keyed_service = static_cast<KeyedService*>(service_.get());
  keyed_service->Shutdown();
  keyed_service->Shutdown();
}

}  // namespace local_ai
