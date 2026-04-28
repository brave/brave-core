// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_batch_passage_embedder.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/passage_embeddings/public/mojom/passage_embeddings.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace passage_embeddings {

namespace {

constexpr double kEmbeddingA[] = {1.0, 2.0, 3.0};
constexpr double kEmbeddingB[] = {4.0, 5.0, 6.0};

std::vector<double> AsVector(const double (&arr)[3]) {
  return {std::begin(arr), std::end(arr)};
}

std::vector<float> AsFloatVector(const double (&arr)[3]) {
  return {static_cast<float>(arr[0]), static_cast<float>(arr[1]),
          static_cast<float>(arr[2])};
}

// Renderer-side fake. By default returns kEmbeddingA for every passage;
// configurable to return per-call canned values, an empty embedding, or
// an out-of-float-range value.
class FakeRendererEmbedder : public local_ai::mojom::PassageEmbedder {
 public:
  void GenerateEmbeddings(const std::string& text,
                          GenerateEmbeddingsCallback callback) override {
    requests_.push_back(text);
    if (!queued_responses_.empty()) {
      auto response = std::move(queued_responses_.front());
      queued_responses_.erase(queued_responses_.begin());
      std::move(callback).Run(std::move(response));
      return;
    }
    std::move(callback).Run(AsVector(kEmbeddingA));
  }

  void AddReceiver(
      mojo::PendingReceiver<local_ai::mojom::PassageEmbedder> receiver) {
    receivers_.Add(this, std::move(receiver));
  }

  void QueueResponse(std::vector<double> response) {
    queued_responses_.push_back(std::move(response));
  }

  const std::vector<std::string>& requests() const { return requests_; }
  void ResetReceivers() { receivers_.Clear(); }

 private:
  std::vector<std::string> requests_;
  std::vector<std::vector<double>> queued_responses_;
  mojo::ReceiverSet<local_ai::mojom::PassageEmbedder> receivers_;
};

// Factory fake. Init replies asynchronously with init_success_; Bind
// forwards the receiver to the renderer worker.
class FakePassageEmbedderFactory
    : public local_ai::mojom::PassageEmbedderFactory {
 public:
  explicit FakePassageEmbedderFactory(FakeRendererEmbedder* worker)
      : worker_(worker) {}

  void Init(local_ai::mojom::ModelFilesPtr model_files,
            InitCallback callback) override {
    ++init_count_;
    if (defer_init_reply_) {
      pending_init_callback_ = std::move(callback);
      return;
    }
    std::move(callback).Run(init_success_);
  }

  void Bind(mojo::PendingReceiver<local_ai::mojom::PassageEmbedder> receiver)
      override {
    ++bind_count_;
    worker_->AddReceiver(std::move(receiver));
  }

  mojo::Remote<local_ai::mojom::PassageEmbedderFactory> BindRemote() {
    mojo::Remote<local_ai::mojom::PassageEmbedderFactory> remote;
    receiver_.Bind(remote.BindNewPipeAndPassReceiver());
    return remote;
  }

  void ResetReceiver() { receiver_.reset(); }

  void set_init_success(bool success) { init_success_ = success; }
  void set_defer_init_reply(bool defer) { defer_init_reply_ = defer; }
  void RunDeferredInit(bool success) {
    ASSERT_TRUE(pending_init_callback_);
    std::move(pending_init_callback_).Run(success);
  }

  int init_count() const { return init_count_; }
  int bind_count() const { return bind_count_; }

 private:
  raw_ptr<FakeRendererEmbedder> worker_;
  bool init_success_ = true;
  bool defer_init_reply_ = false;
  InitCallback pending_init_callback_;
  int init_count_ = 0;
  int bind_count_ = 0;
  mojo::Receiver<local_ai::mojom::PassageEmbedderFactory> receiver_{this};
};

}  // namespace

class BraveBatchPassageEmbedderTest : public testing::Test {
 protected:
  // Constructs the embedder under test; populates load_success_ via
  // load_callback and disconnect_called_ via on_disconnect.
  void CreateEmbedder() {
    embedder_ = std::make_unique<BraveBatchPassageEmbedder>(
        embedder_remote_.BindNewPipeAndPassReceiver(), factory_.BindRemote(),
        local_ai::mojom::ModelFiles::New(), load_success_.GetCallback(),
        base::BindOnce(
            [](base::test::TestFuture<void>* future) { future->SetValue(); },
            &disconnect_called_));
  }

  base::test::TaskEnvironment task_environment_;
  FakeRendererEmbedder fake_worker_;
  FakePassageEmbedderFactory factory_{&fake_worker_};
  mojo::Remote<mojom::PassageEmbedder> embedder_remote_;
  base::test::TestFuture<bool> load_success_;
  base::test::TestFuture<void> disconnect_called_;
  std::unique_ptr<BraveBatchPassageEmbedder> embedder_;
};

TEST_F(BraveBatchPassageEmbedderTest, InitSuccessSignalsLoadAndBinds) {
  CreateEmbedder();
  EXPECT_TRUE(load_success_.Get());
  // factory_->Bind is fired from OnInitDone before load_success runs;
  // wait for the message to reach FakePassageEmbedderFactory.
  ASSERT_TRUE(base::test::RunUntil([&] { return factory_.bind_count() > 0; }));
  EXPECT_EQ(1, factory_.init_count());
  EXPECT_EQ(1, factory_.bind_count());
  EXPECT_FALSE(disconnect_called_.IsReady());
}

TEST_F(BraveBatchPassageEmbedderTest,
       InitFailureSignalsLoadFalseAndDisconnect) {
  factory_.set_init_success(false);
  CreateEmbedder();
  EXPECT_FALSE(load_success_.Get());
  EXPECT_TRUE(disconnect_called_.Wait());
  EXPECT_EQ(0, factory_.bind_count());
}

TEST_F(BraveBatchPassageEmbedderTest, EmptyPassagesShortCircuit) {
  CreateEmbedder();
  ASSERT_TRUE(load_success_.Get());

  base::test::TestFuture<std::vector<mojom::PassageEmbeddingsResultPtr>> result;
  embedder_remote_->GenerateEmbeddings({}, mojom::PassagePriority::kPassive,
                                       result.GetCallback());
  EXPECT_TRUE(result.Get().empty());
  EXPECT_TRUE(fake_worker_.requests().empty());
}

TEST_F(BraveBatchPassageEmbedderTest, BatchReturnsResultsInOrder) {
  CreateEmbedder();
  ASSERT_TRUE(load_success_.Get());

  fake_worker_.QueueResponse(AsVector(kEmbeddingA));
  fake_worker_.QueueResponse(AsVector(kEmbeddingB));

  base::test::TestFuture<std::vector<mojom::PassageEmbeddingsResultPtr>> result;
  embedder_remote_->GenerateEmbeddings({"first", "second"},
                                       mojom::PassagePriority::kPassive,
                                       result.GetCallback());
  auto results = result.Take();
  ASSERT_EQ(2u, results.size());
  EXPECT_EQ(AsFloatVector(kEmbeddingA), results[0]->embeddings);
  EXPECT_EQ(AsFloatVector(kEmbeddingB), results[1]->embeddings);
  EXPECT_EQ(std::vector<std::string>({"first", "second"}),
            fake_worker_.requests());
}

TEST_F(BraveBatchPassageEmbedderTest, ConcurrentBatchesAreSerialized) {
  CreateEmbedder();
  ASSERT_TRUE(load_success_.Get());

  base::test::TestFuture<std::vector<mojom::PassageEmbeddingsResultPtr>> a;
  base::test::TestFuture<std::vector<mojom::PassageEmbeddingsResultPtr>> b;
  embedder_remote_->GenerateEmbeddings({"a"}, mojom::PassagePriority::kPassive,
                                       a.GetCallback());
  embedder_remote_->GenerateEmbeddings({"b"}, mojom::PassagePriority::kPassive,
                                       b.GetCallback());

  EXPECT_EQ(1u, a.Get().size());
  EXPECT_EQ(1u, b.Get().size());
  EXPECT_EQ(std::vector<std::string>({"a", "b"}), fake_worker_.requests());
}

TEST_F(BraveBatchPassageEmbedderTest, EmptyEmbeddingFailsBatch) {
  CreateEmbedder();
  ASSERT_TRUE(load_success_.Get());

  fake_worker_.QueueResponse(AsVector(kEmbeddingA));
  fake_worker_.QueueResponse({});  // second passage returns empty

  base::test::TestFuture<std::vector<mojom::PassageEmbeddingsResultPtr>> result;
  embedder_remote_->GenerateEmbeddings({"first", "second"},
                                       mojom::PassagePriority::kPassive,
                                       result.GetCallback());
  EXPECT_TRUE(result.Get().empty());
}

TEST_F(BraveBatchPassageEmbedderTest, OutOfRangeFloatFailsBatch) {
  CreateEmbedder();
  ASSERT_TRUE(load_success_.Get());

  // Value above max float overflows on cast.
  fake_worker_.QueueResponse({std::numeric_limits<double>::max()});

  base::test::TestFuture<std::vector<mojom::PassageEmbeddingsResultPtr>> result;
  embedder_remote_->GenerateEmbeddings(
      {"too-big"}, mojom::PassagePriority::kPassive, result.GetCallback());
  EXPECT_TRUE(result.Get().empty());
}

TEST_F(BraveBatchPassageEmbedderTest, FactoryDisconnectBeforeInitFailsLoad) {
  factory_.set_defer_init_reply(true);
  CreateEmbedder();
  EXPECT_FALSE(load_success_.IsReady());

  factory_.ResetReceiver();

  EXPECT_FALSE(load_success_.Get());
  EXPECT_TRUE(disconnect_called_.Wait());
}

TEST_F(BraveBatchPassageEmbedderTest, RendererDisconnectFiresOnDisconnect) {
  CreateEmbedder();
  ASSERT_TRUE(load_success_.Get());
  // Ensure factory_->Bind has reached the worker before we reset its
  // receiver set; otherwise ResetReceivers runs before AddReceiver and
  // the disconnect we drive below never happens.
  ASSERT_TRUE(base::test::RunUntil([&] { return factory_.bind_count() > 0; }));

  fake_worker_.ResetReceivers();
  EXPECT_TRUE(disconnect_called_.Wait());
}

TEST_F(BraveBatchPassageEmbedderTest, ReceiverDisconnectFiresOnDisconnect) {
  CreateEmbedder();
  ASSERT_TRUE(load_success_.Get());

  // Drop the caller-side remote; the embedder's receiver disconnects.
  embedder_remote_.reset();

  EXPECT_TRUE(disconnect_called_.Wait());
}

}  // namespace passage_embeddings
