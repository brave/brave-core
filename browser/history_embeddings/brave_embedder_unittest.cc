// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_embedder.h"

#include <string>
#include <vector>

#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace passage_embeddings {

namespace {

// Fake PassageEmbedder that records the order of passages received and
// can hold responses until explicitly released.
class FakePassageEmbedder : public local_ai::mojom::PassageEmbedder {
 public:
  void GenerateEmbeddings(const std::string& text,
                          GenerateEmbeddingsCallback callback) override {
    passages_received_.push_back(text);
    if (hold_responses_) {
      held_callbacks_.push_back(std::move(callback));
    } else {
      std::move(callback).Run({1.0, 2.0, 3.0});
    }
  }

  void ReleaseNextResponse() {
    ASSERT_FALSE(held_callbacks_.empty());
    auto callback = std::move(held_callbacks_.front());
    held_callbacks_.erase(held_callbacks_.begin());
    std::move(callback).Run({1.0, 2.0, 3.0});
  }

  void set_hold_responses(bool hold) { hold_responses_ = hold; }
  const std::vector<std::string>& passages_received() const {
    return passages_received_;
  }

 private:
  bool hold_responses_ = false;
  std::vector<std::string> passages_received_;
  std::vector<GenerateEmbeddingsCallback> held_callbacks_;
};

using EmbeddingResult = std::tuple<std::vector<std::string>,
                                   std::vector<Embedding>,
                                   Embedder::TaskId,
                                   ComputeEmbeddingsStatus>;

}  // namespace

class BraveEmbedderTest : public testing::Test {
 protected:
  void SetUp() override {
    mojo::PendingRemote<local_ai::mojom::PassageEmbedder> remote;
    receiver_.Bind(remote.InitWithNewPipeAndPassReceiver());
    embedder_ = std::make_unique<BraveEmbedder>(std::move(remote));
  }

  base::test::TaskEnvironment task_environment_;
  FakePassageEmbedder fake_worker_;
  mojo::Receiver<local_ai::mojom::PassageEmbedder> receiver_{&fake_worker_};
  std::unique_ptr<BraveEmbedder> embedder_;
};

TEST_F(BraveEmbedderTest, BasicEmbedding) {
  base::test::TestFuture<std::vector<std::string>, std::vector<Embedding>,
                         Embedder::TaskId, ComputeEmbeddingsStatus>
      future;
  embedder_->ComputePassagesEmbeddings(PassagePriority::kPassive,
                                       {"hello world"}, future.GetCallback());
  auto [passages, embeddings, task_id, status] = future.Take();
  EXPECT_EQ(status, ComputeEmbeddingsStatus::kSuccess);
  ASSERT_EQ(embeddings.size(), 1u);
}

TEST_F(BraveEmbedderTest, HighPriorityPreemptsLowPriority) {
  // Hold responses so we can control when each completes.
  fake_worker_.set_hold_responses(true);

  // Submit a passive (low-priority) job with 3 passages.
  base::test::TestFuture<std::vector<std::string>, std::vector<Embedding>,
                         Embedder::TaskId, ComputeEmbeddingsStatus>
      passive_future;
  embedder_->ComputePassagesEmbeddings(PassagePriority::kPassive,
                                       {"p1", "p2", "p3"},
                                       passive_future.GetCallback());

  // Let the first passage of the passive job be submitted.
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return fake_worker_.passages_received().size() >= 1u; }));
  EXPECT_EQ(fake_worker_.passages_received()[0], "p1");

  // Now submit a high-priority (search) job while passive is in-flight.
  base::test::TestFuture<std::vector<std::string>, std::vector<Embedding>,
                         Embedder::TaskId, ComputeEmbeddingsStatus>
      search_future;
  embedder_->ComputePassagesEmbeddings(PassagePriority::kUserInitiated,
                                       {"search query"},
                                       search_future.GetCallback());

  // Complete the in-flight passive passage.
  fake_worker_.ReleaseNextResponse();
  // The next passage submitted should be the search query (high priority),
  // not p2 (the next passive passage).
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return fake_worker_.passages_received().size() >= 2u; }));
  EXPECT_EQ(fake_worker_.passages_received()[1], "search query");

  // Complete the search query.
  fake_worker_.ReleaseNextResponse();
  ASSERT_TRUE(base::test::RunUntil([&] { return search_future.IsReady(); }));
  auto [s_passages, s_embeddings, s_task_id, s_status] = search_future.Take();
  EXPECT_EQ(s_status, ComputeEmbeddingsStatus::kSuccess);
  ASSERT_EQ(s_embeddings.size(), 1u);

  // Now the remaining passive passages should continue.
  // Release p2 and p3.
  fake_worker_.ReleaseNextResponse();
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return fake_worker_.passages_received().size() >= 3u; }));
  fake_worker_.ReleaseNextResponse();
  ASSERT_TRUE(base::test::RunUntil([&] { return passive_future.IsReady(); }));
  auto [p_passages, p_embeddings, p_task_id, p_status] = passive_future.Take();
  EXPECT_EQ(p_status, ComputeEmbeddingsStatus::kSuccess);
  ASSERT_EQ(p_embeddings.size(), 3u);

  // Verify full order: p1, search query, p2, p3
  ASSERT_EQ(fake_worker_.passages_received().size(), 4u);
  EXPECT_EQ(fake_worker_.passages_received()[2], "p2");
  EXPECT_EQ(fake_worker_.passages_received()[3], "p3");
}

TEST_F(BraveEmbedderTest, TryCancelPendingJob) {
  fake_worker_.set_hold_responses(true);

  // Submit two jobs.
  base::test::TestFuture<std::vector<std::string>, std::vector<Embedding>,
                         Embedder::TaskId, ComputeEmbeddingsStatus>
      future1;
  auto task1 = embedder_->ComputePassagesEmbeddings(
      PassagePriority::kPassive, {"a"}, future1.GetCallback());

  base::test::TestFuture<std::vector<std::string>, std::vector<Embedding>,
                         Embedder::TaskId, ComputeEmbeddingsStatus>
      future2;
  auto task2 = embedder_->ComputePassagesEmbeddings(
      PassagePriority::kPassive, {"b"}, future2.GetCallback());

  ASSERT_TRUE(base::test::RunUntil(
      [&] { return fake_worker_.passages_received().size() >= 1u; }));

  // task1 is in-flight, cannot cancel.
  EXPECT_FALSE(embedder_->TryCancel(task1));
  // task2 is pending, can cancel.
  EXPECT_TRUE(embedder_->TryCancel(task2));

  ASSERT_TRUE(future2.IsReady());
  EXPECT_EQ(std::get<3>(future2.Take()), ComputeEmbeddingsStatus::kCanceled);

  // Complete task1.
  fake_worker_.ReleaseNextResponse();
  ASSERT_TRUE(base::test::RunUntil([&] { return future1.IsReady(); }));
  EXPECT_EQ(std::get<3>(future1.Take()), ComputeEmbeddingsStatus::kSuccess);
}

TEST_F(BraveEmbedderTest, EmptyPassagesReturnsImmediately) {
  base::test::TestFuture<std::vector<std::string>, std::vector<Embedding>,
                         Embedder::TaskId, ComputeEmbeddingsStatus>
      future;
  embedder_->ComputePassagesEmbeddings(PassagePriority::kPassive, {},
                                       future.GetCallback());
  auto [passages, embeddings, task_id, status] = future.Take();
  EXPECT_EQ(status, ComputeEmbeddingsStatus::kSuccess);
  EXPECT_TRUE(embeddings.empty());
}

}  // namespace passage_embeddings
