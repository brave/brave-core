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
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
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
// queue per-call canned responses to simulate empty / out-of-range
// returns.
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

  mojo::PendingRemote<local_ai::mojom::PassageEmbedder> BindNewRemote() {
    mojo::PendingRemote<local_ai::mojom::PassageEmbedder> remote;
    receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
    return remote;
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

}  // namespace

class BraveBatchPassageEmbedderTest : public testing::Test {
 protected:
  // Constructs the embedder under test wired to fake_worker_.
  void CreateEmbedder() {
    embedder_ = std::make_unique<BraveBatchPassageEmbedder>(
        embedder_remote_.BindNewPipeAndPassReceiver(),
        fake_worker_.BindNewRemote(),
        base::BindOnce(
            [](base::test::TestFuture<void>* future) { future->SetValue(); },
            &disconnect_called_));
  }

  base::test::TaskEnvironment task_environment_;
  FakeRendererEmbedder fake_worker_;
  mojo::Remote<mojom::PassageEmbedder> embedder_remote_;
  base::test::TestFuture<void> disconnect_called_;
  std::unique_ptr<BraveBatchPassageEmbedder> embedder_;
};

TEST_F(BraveBatchPassageEmbedderTest, EmptyPassagesShortCircuit) {
  CreateEmbedder();
  base::test::TestFuture<std::vector<mojom::PassageEmbeddingsResultPtr>> result;
  embedder_remote_->GenerateEmbeddings({}, mojom::PassagePriority::kPassive,
                                       result.GetCallback());
  EXPECT_TRUE(result.Get().empty());
  EXPECT_TRUE(fake_worker_.requests().empty());
}

TEST_F(BraveBatchPassageEmbedderTest, BatchReturnsResultsInOrder) {
  CreateEmbedder();
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
  // Value above max float overflows on cast.
  fake_worker_.QueueResponse({std::numeric_limits<double>::max()});

  base::test::TestFuture<std::vector<mojom::PassageEmbeddingsResultPtr>> result;
  embedder_remote_->GenerateEmbeddings(
      {"too-big"}, mojom::PassagePriority::kPassive, result.GetCallback());
  EXPECT_TRUE(result.Get().empty());
}

TEST_F(BraveBatchPassageEmbedderTest, RendererDisconnectFiresOnDisconnect) {
  CreateEmbedder();
  fake_worker_.ResetReceivers();
  EXPECT_TRUE(disconnect_called_.Wait());
}

TEST_F(BraveBatchPassageEmbedderTest, ReceiverDisconnectFiresOnDisconnect) {
  CreateEmbedder();
  embedder_remote_.reset();
  EXPECT_TRUE(disconnect_called_.Wait());
}

}  // namespace passage_embeddings
