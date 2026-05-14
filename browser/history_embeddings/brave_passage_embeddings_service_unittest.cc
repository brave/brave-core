// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service.h"

#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/browser/history_embeddings/brave_batch_passage_embedder.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
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

// Acts as the renderer-side LocalAIService consumer in the test:
// receives a LocalAIService PendingReceiver from the embedder via
// BindLocalAIReceiver and calls RegisterPassageEmbedderFactory back
// through it.
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
    service_ =
        std::make_unique<BravePassageEmbeddingsService>(base::BindRepeating(
            &BravePassageEmbeddingsServiceTest::CreateFakeWebContents,
            base::Unretained(this)));
  }

  void TearDown() override { service_.reset(); }

  void CreateFakeWebContents(
      local_ai::BackgroundWebContents::Delegate* delegate,
      BraveBatchPassageEmbedder::BackgroundWebContentsCreatedCallback
          callback) {
    last_delegate_ = delegate;
    // The delegate is the embedder which owns this BG contents — they
    // die together. Clear both raw_ptrs from the destructor so neither is
    // left dangling when the service drops batch_embedder_.
    auto web_contents = std::make_unique<FakeBackgroundWebContents>(
        delegate, base::BindOnce(
                      [](raw_ptr<FakeBackgroundWebContents>* contents_ref,
                         raw_ptr<local_ai::BackgroundWebContents::Delegate>*
                             delegate_ref) {
                        *contents_ref = nullptr;
                        *delegate_ref = nullptr;
                      },
                      &last_created_web_contents_, &last_delegate_));
    last_created_web_contents_ = web_contents.get();
    std::move(callback).Run(std::move(web_contents));
  }

  // Drives the renderer-side half of the load: opens a LocalAIService
  // pipe to the embedder (the way UntrustedLocalAIUI does in
  // production) and registers the fake factory. Requires the
  // embedder to have already created its background contents.
  void RegisterFactory() {
    ASSERT_TRUE(last_delegate_);
    auto* embedder = static_cast<BraveBatchPassageEmbedder*>(last_delegate_);
    mojo::Remote<local_ai::mojom::LocalAIService> local_ai_remote;
    embedder->BindLocalAIReceiver(local_ai_remote.BindNewPipeAndPassReceiver());
    local_ai_remote->RegisterPassageEmbedderFactory(fake_factory_.BindRemote());
    local_ai_remote.FlushForTesting();
  }

  struct LoadResult {
    base::test::TestFuture<bool> load_success;
    mojo::Remote<mojom::PassageEmbedder> embedder;
  };
  std::unique_ptr<LoadResult> IssueLoad() {
    auto result = std::make_unique<LoadResult>();
    service_->BindPassageEmbedder(result->embedder.BindNewPipeAndPassReceiver(),
                                  local_ai::mojom::ModelFiles::New(),
                                  result->load_success.GetCallback());
    return result;
  }

  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<BravePassageEmbeddingsService> service_;
  FakeRendererEmbedder fake_worker_;
  FakePassageEmbedderFactory fake_factory_{&fake_worker_};
  raw_ptr<FakeBackgroundWebContents> last_created_web_contents_ = nullptr;
  raw_ptr<local_ai::BackgroundWebContents::Delegate> last_delegate_ = nullptr;
};

TEST_F(BravePassageEmbeddingsServiceTest, BindCreatesBackgroundContents) {
  auto load = IssueLoad();
  EXPECT_TRUE(last_created_web_contents_);
  EXPECT_FALSE(load->load_success.IsReady());
}

TEST_F(BravePassageEmbeddingsServiceTest, BindCompletesOnFactoryRegistration) {
  auto load = IssueLoad();
  ASSERT_TRUE(last_created_web_contents_);

  RegisterFactory();
  ASSERT_TRUE(
      base::test::RunUntil([&] { return fake_factory_.init_count() > 0; }));
  EXPECT_TRUE(load->load_success.Get());
}

TEST_F(BravePassageEmbeddingsServiceTest, EndToEndBatchGenerateEmbeddings) {
  auto load = IssueLoad();
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
  RegisterFactory();

  ASSERT_TRUE(
      base::test::RunUntil([&] { return fake_factory_.init_count() > 0; }));
  EXPECT_FALSE(load->load_success.Get());
  // Disconnect tears the embedder down, releasing its background
  // contents.
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return last_created_web_contents_ == nullptr; }));
}

TEST_F(BravePassageEmbeddingsServiceTest, BackgroundContentsDestroyedCloses) {
  auto load = IssueLoad();
  RegisterFactory();
  ASSERT_TRUE(load->load_success.Get());
  ASSERT_TRUE(last_created_web_contents_);

  last_created_web_contents_->SimulateDestroyed();

  // A new load should recreate background contents.
  auto load2 = IssueLoad();
  EXPECT_TRUE(last_created_web_contents_);
}

TEST_F(BravePassageEmbeddingsServiceTest,
       DuplicateRegisterPassageEmbedderFactoryIgnored) {
  // A buggy renderer that calls RegisterPassageEmbedderFactory twice
  // without first disconnecting must not crash the browser. The
  // embedder should ignore the duplicate and keep the first
  // factory.
  auto load = IssueLoad();
  ASSERT_TRUE(last_created_web_contents_);

  RegisterFactory();

  // Second registration: bind a throwaway remote and pass it in.
  // mojo::Remote::Bind CHECKs when already bound, so without the
  // duplicate guard this would crash the browser.
  ASSERT_TRUE(last_delegate_);
  auto* embedder =
      static_cast<BraveBatchPassageEmbedder*>(last_delegate_.get());
  mojo::Remote<local_ai::mojom::LocalAIService> local_ai_remote;
  embedder->BindLocalAIReceiver(local_ai_remote.BindNewPipeAndPassReceiver());
  mojo::PendingRemote<local_ai::mojom::PassageEmbedderFactory> dummy;
  std::ignore = dummy.InitWithNewPipeAndPassReceiver();
  local_ai_remote->RegisterPassageEmbedderFactory(std::move(dummy));
  local_ai_remote.FlushForTesting();

  ASSERT_TRUE(
      base::test::RunUntil([&] { return fake_factory_.init_count() > 0; }));
  EXPECT_TRUE(load->load_success.Get());
  EXPECT_EQ(1, fake_factory_.init_count());
}

TEST_F(BravePassageEmbeddingsServiceTest, BindAgainAfterEmbedderRemoteReset) {
  auto load = IssueLoad();
  RegisterFactory();
  ASSERT_TRUE(load->load_success.Get());
  ASSERT_TRUE(last_created_web_contents_);

  // Drop the caller-side embedder remote. The embedder's receiver
  // disconnects, on_disconnect fires up to the service, and the
  // service drops the embedder (which tears down its contents).
  load->embedder.reset();
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return last_created_web_contents_ == nullptr; }));

  // The factory pipe was reset along with the embedder; the fake
  // still holds a now-disconnected receiver, so reset it before
  // re-binding for the next cycle.
  fake_factory_.ResetReceiver();

  auto load2 = IssueLoad();
  ASSERT_TRUE(last_created_web_contents_);
  RegisterFactory();
  ASSERT_TRUE(
      base::test::RunUntil([&] { return fake_factory_.init_count() > 1; }));
  EXPECT_TRUE(load2->load_success.Get());
}

TEST_F(BravePassageEmbeddingsServiceTest, BindLocalAIReceiverNoopWithoutBatch) {
  // Service-level forwarder is a no-op when no BatchEmbedder is bound.
  // The receiver is dropped; the test just confirms the call doesn't
  // crash and the remote sees a disconnect.
  mojo::Remote<local_ai::mojom::LocalAIService> remote;
  service_->BindLocalAIReceiver(remote.BindNewPipeAndPassReceiver());
  base::test::TestFuture<void> disconnected;
  remote.set_disconnect_handler(disconnected.GetCallback());
  EXPECT_TRUE(disconnected.Wait());
}

TEST_F(BravePassageEmbeddingsServiceTest, BindLocalAIReceiverForwardsToBatch) {
  // Once a BatchEmbedder is alive, the forwarder hands the receiver to
  // it — RegisterPassageEmbedderFactory then drives the load to Ready.
  auto load = IssueLoad();
  ASSERT_TRUE(last_created_web_contents_);
  ASSERT_TRUE(last_delegate_);

  mojo::Remote<local_ai::mojom::LocalAIService> local_ai_remote;
  service_->BindLocalAIReceiver(local_ai_remote.BindNewPipeAndPassReceiver());
  local_ai_remote->RegisterPassageEmbedderFactory(fake_factory_.BindRemote());
  local_ai_remote.FlushForTesting();

  ASSERT_TRUE(
      base::test::RunUntil([&] { return fake_factory_.init_count() > 0; }));
  EXPECT_TRUE(load->load_success.Get());
}

}  // namespace passage_embeddings
