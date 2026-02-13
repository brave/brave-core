// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/core/local_ai_service.h"

#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

namespace {

constexpr double kTestEmbeddingData[] = {1.0, 2.0, 3.0};

std::vector<double> TestEmbedding() {
  return {std::begin(kTestEmbeddingData), std::end(kTestEmbeddingData)};
}

// Fake model worker that returns a fixed embedding vector.
class FakeModelWorker : public mojom::OnDeviceModelWorker {
 public:
  void GenerateEmbeddings(const std::string& input,
                          GenerateEmbeddingsCallback callback) override {
    embed_count_++;
    std::move(callback).Run(TestEmbedding());
  }

  mojo::PendingRemote<mojom::OnDeviceModelWorker> BindNewPipeAndPassRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void Reset() { receiver_.reset(); }

  int embed_count() const { return embed_count_; }

 private:
  int embed_count_ = 0;
  mojo::Receiver<mojom::OnDeviceModelWorker> receiver_{this};
};

// Fake BackgroundWebContents that stores the delegate so tests can trigger
// lifecycle events manually.
class FakeBackgroundWebContents : public BackgroundWebContents {
 public:
  FakeBackgroundWebContents(Delegate* delegate,
                            base::OnceClosure on_destroyed)
      : delegate_(delegate), on_destroyed_(std::move(on_destroyed)) {}
  ~FakeBackgroundWebContents() override { std::move(on_destroyed_).Run(); }

  void SimulateReady() { delegate_->OnBackgroundContentsReady(); }
  void SimulateDestroyed() {
    delegate_->OnBackgroundContentsDestroyed(DestroyReason::kRendererGone);
  }

 private:
  raw_ptr<Delegate> delegate_;
  base::OnceClosure on_destroyed_;
};

}  // namespace

class LocalAIServiceTest : public testing::Test {
 protected:
  void SetUp() override {
    service_ = std::make_unique<LocalAIService>(base::BindRepeating(
        &LocalAIServiceTest::CreateFakeWebContents,
        base::Unretained(this)));
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
            [](raw_ptr<FakeBackgroundWebContents>* ref) {
              *ref = nullptr;
            },
            &last_created_web_contents_));
    last_created_web_contents_ = web_contents.get();
    return web_contents;
  }

  void BindFakeModelWorker() {
    service_->RegisterOnDeviceModelWorker(
        fake_model_worker_.BindNewPipeAndPassRemote());
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<LocalAIService> service_;
  FakeModelWorker fake_model_worker_;
  raw_ptr<FakeBackgroundWebContents> last_created_web_contents_ = nullptr;
};

TEST_F(LocalAIServiceTest, GenerateEmbeddingsCreatesBackgroundContents) {
  base::test::TestFuture<const std::vector<double>&> future;
  service_->GenerateEmbeddings("test", future.GetCallback());

  EXPECT_TRUE(last_created_web_contents_);
  EXPECT_FALSE(future.IsReady());
}

TEST_F(LocalAIServiceTest, GenerateEmbeddingsQueuesWhenNotReady) {
  base::test::TestFuture<const std::vector<double>&> future1;
  base::test::TestFuture<const std::vector<double>&> future2;

  service_->GenerateEmbeddings("hello", future1.GetCallback());
  service_->GenerateEmbeddings("world", future2.GetCallback());

  // Both should be queued, not resolved.
  EXPECT_FALSE(future1.IsReady());
  EXPECT_FALSE(future2.IsReady());
}

TEST_F(LocalAIServiceTest, RegisterModelWorkerProcessesPendingRequests) {
  base::test::TestFuture<const std::vector<double>&> future1;
  base::test::TestFuture<const std::vector<double>&> future2;

  service_->GenerateEmbeddings("hello", future1.GetCallback());
  service_->GenerateEmbeddings("world", future2.GetCallback());

  BindFakeModelWorker();

  EXPECT_EQ(TestEmbedding(), future1.Get());
  EXPECT_EQ(TestEmbedding(), future2.Get());
  EXPECT_EQ(2, fake_model_worker_.embed_count());
}

TEST_F(LocalAIServiceTest, GenerateEmbeddingsForwardsDirectlyWhenReady) {
  BindFakeModelWorker();

  service_->GenerateEmbeddings("test", base::DoNothing());

  base::test::TestFuture<const std::vector<double>&> future;
  service_->GenerateEmbeddings("direct", future.GetCallback());

  EXPECT_EQ(TestEmbedding(), future.Get());
}

TEST_F(LocalAIServiceTest, ShutdownFailsPendingRequests) {
  base::test::TestFuture<const std::vector<double>&> future;
  service_->GenerateEmbeddings("pending", future.GetCallback());

  static_cast<KeyedService*>(service_.get())->Shutdown();

  // Pending request should be resolved with empty vector.
  EXPECT_EQ(std::vector<double>{}, future.Get());
}

TEST_F(LocalAIServiceTest, OnBackgroundContentsDestroyedFailsPending) {
  base::test::TestFuture<const std::vector<double>&> future;
  service_->GenerateEmbeddings("pending", future.GetCallback());

  ASSERT_TRUE(last_created_web_contents_);
  last_created_web_contents_->SimulateDestroyed();

  EXPECT_EQ(std::vector<double>{}, future.Get());
}

TEST_F(LocalAIServiceTest, ReinitializesAfterDestroyed) {
  // Trigger background contents creation and bind embedder.
  base::test::TestFuture<const std::vector<double>&> setup_future;
  service_->GenerateEmbeddings("setup", setup_future.GetCallback());
  BindFakeModelWorker();
  EXPECT_EQ(TestEmbedding(), setup_future.Get());

  ASSERT_TRUE(last_created_web_contents_);
  last_created_web_contents_->SimulateDestroyed();

  // A new GenerateEmbeddings() call should queue (not crash) since
  // state was reset.
  base::test::TestFuture<const std::vector<double>&> future;
  service_->GenerateEmbeddings("after-crash", future.GetCallback());

  EXPECT_FALSE(future.IsReady());
}

TEST_F(LocalAIServiceTest, DoubleShutdownIsIdempotent) {
  base::test::TestFuture<const std::vector<double>&> future;
  service_->GenerateEmbeddings("pending", future.GetCallback());

  auto* keyed_service = static_cast<KeyedService*>(service_.get());
  keyed_service->Shutdown();
  keyed_service->Shutdown();

  EXPECT_EQ(std::vector<double>{}, future.Get());
}

}  // namespace local_ai
