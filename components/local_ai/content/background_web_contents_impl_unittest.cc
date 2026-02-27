// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/content/background_web_contents_impl.h"

#include <memory>
#include <string>
#include <vector>

#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace local_ai {

namespace {

constexpr double kTestEmbeddingData[] = {1.0, 2.0, 3.0};

std::vector<double> TestEmbedding() {
  return {std::begin(kTestEmbeddingData), std::end(kTestEmbeddingData)};
}

class FakeWorker : public mojom::PassageEmbedder {
 public:
  void GenerateEmbeddings(const std::string& text,
                          GenerateEmbeddingsCallback callback) override {
    std::move(callback).Run(TestEmbedding());
  }

  mojo::PendingRemote<mojom::PassageEmbedder> BindNewPipeAndPassRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

 private:
  mojo::Receiver<mojom::PassageEmbedder> receiver_{this};
};

}  // namespace

class MockDelegate : public BackgroundWebContents::Delegate {
 public:
  MOCK_METHOD(void, OnBackgroundContentsReady, (), (override));
  MOCK_METHOD(void,
              OnBackgroundContentsDestroyed,
              (BackgroundWebContents::DestroyReason reason),
              (override));
};

class BackgroundWebContentsImplTest
    : public content::RenderViewHostTestHarness {
 protected:
  void TearDown() override {
    background_contents_.reset();
    content::RenderViewHostTestHarness::TearDown();
  }

  void CreateContents(const base::Location& location = FROM_HERE) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    background_contents_ = std::make_unique<BackgroundWebContentsImpl>(
        browser_context(), GURL("chrome-untrusted://test/"), &delegate_);
  }

  // Navigate the background WebContents to trigger DidFinishLoad.
  void SimulatePageLoad(const base::Location& location = FROM_HERE) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    auto* wc = background_contents_->web_contents();
    ASSERT_NE(nullptr, wc);
    content::NavigationSimulator::NavigateAndCommitFromBrowser(
        wc, GURL("chrome-untrusted://test/"));
  }

  testing::NiceMock<MockDelegate> delegate_;
  std::unique_ptr<BackgroundWebContentsImpl> background_contents_;
};

TEST_F(BackgroundWebContentsImplTest, ConstructionCreatesWebContents) {
  CreateContents();
  EXPECT_NE(nullptr, background_contents_->web_contents());
}

TEST_F(BackgroundWebContentsImplTest, DidFinishLoadCallsReady) {
  CreateContents();

  EXPECT_CALL(delegate_, OnBackgroundContentsReady());

  SimulatePageLoad();
}

TEST_F(BackgroundWebContentsImplTest,
       CloseContentsCallsOnBackgroundContentsDestroyed) {
  CreateContents();

  EXPECT_CALL(delegate_, OnBackgroundContentsDestroyed(
                             BackgroundWebContents::DestroyReason::kClose));

  // CloseContents is called when window.close() fires.
  static_cast<content::WebContentsDelegate*>(background_contents_.get())
      ->CloseContents(background_contents_->web_contents());
}

TEST_F(BackgroundWebContentsImplTest, UnexpectedUrlCallsDestroyed) {
  CreateContents();

  EXPECT_CALL(delegate_, OnBackgroundContentsReady()).Times(0);
  EXPECT_CALL(delegate_,
              OnBackgroundContentsDestroyed(
                  BackgroundWebContents::DestroyReason::kInvalidUrl));

  // Navigate to a different URL than expected.
  auto* wc = background_contents_->web_contents();
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      wc, GURL("chrome-untrusted://wrong/"));
}

TEST_F(BackgroundWebContentsImplTest, DestructorDoesNotFireDelegateCallbacks) {
  CreateContents();
  // Destroying should not crash or fire delegate callbacks.
  background_contents_.reset();
}

TEST_F(BackgroundWebContentsImplTest,
       BindNewPassageEmbedderBeforeWorkerQueues) {
  CreateContents();

  // Get an embedder remote before any worker is registered.
  auto embedder_remote = background_contents_->BindNewPassageEmbedder();
  EXPECT_TRUE(embedder_remote.is_valid());

  // Register the worker — pending receiver should be bound.
  FakeWorker fake_worker;
  background_contents_->SetWorkerRemote(fake_worker.BindNewPipeAndPassRemote());

  // Now a GenerateEmbeddings call should work.
  mojo::Remote<mojom::PassageEmbedder> embedder;
  embedder.Bind(std::move(embedder_remote));

  base::test::TestFuture<const std::vector<double>&> future;
  embedder->GenerateEmbeddings("test", future.GetCallback());
  EXPECT_EQ(TestEmbedding(), future.Get());
}

TEST_F(BackgroundWebContentsImplTest,
       BindNewPassageEmbedderAfterWorkerBindsImmediately) {
  CreateContents();

  FakeWorker fake_worker;
  background_contents_->SetWorkerRemote(fake_worker.BindNewPipeAndPassRemote());

  auto embedder_remote = background_contents_->BindNewPassageEmbedder();
  mojo::Remote<mojom::PassageEmbedder> embedder;
  embedder.Bind(std::move(embedder_remote));

  base::test::TestFuture<const std::vector<double>&> future;
  embedder->GenerateEmbeddings("direct", future.GetCallback());
  EXPECT_EQ(TestEmbedding(), future.Get());
}

TEST_F(BackgroundWebContentsImplTest, MultipleConsumersEachGetResults) {
  CreateContents();

  FakeWorker fake_worker;
  background_contents_->SetWorkerRemote(fake_worker.BindNewPipeAndPassRemote());

  // Bind two independent consumer remotes.
  mojo::Remote<mojom::PassageEmbedder> embedder1;
  embedder1.Bind(background_contents_->BindNewPassageEmbedder());
  mojo::Remote<mojom::PassageEmbedder> embedder2;
  embedder2.Bind(background_contents_->BindNewPassageEmbedder());

  base::test::TestFuture<const std::vector<double>&> future1;
  base::test::TestFuture<const std::vector<double>&> future2;
  embedder1->GenerateEmbeddings("from consumer 1", future1.GetCallback());
  embedder2->GenerateEmbeddings("from consumer 2", future2.GetCallback());

  EXPECT_EQ(TestEmbedding(), future1.Get());
  EXPECT_EQ(TestEmbedding(), future2.Get());
}

}  // namespace local_ai
