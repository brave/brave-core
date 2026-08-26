// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/history_embeddings/content/open_tab_passages.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "components/feature_engagement/test/mock_tracker.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/test/history_service_test_util.h"
#include "components/history_embeddings/content/history_embeddings_service.h"
#include "components/history_embeddings/core/mock_answerer.h"
#include "components/history_embeddings/core/mock_intent_classifier.h"
#include "components/history_embeddings/core/vector_database.h"
#include "components/optimization_guide/core/delivery/test_optimization_guide_model_provider.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/page_content_annotations/content/page_content_extraction_service.h"
#include "components/page_content_annotations/content/page_embeddings_service.h"
#include "components/page_content_annotations/core/test_page_content_annotations_service.h"
#include "components/passage_embeddings/core/passage_embeddings_test_util.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace history_embeddings {

namespace {

constexpr size_t kMaxPassagesPerUrl = 2;
constexpr size_t kMaxPassageBytes = 16;

}  // namespace

// `HistoryEmbeddingsService` befriends this exact name so its own tests can
// reach protected members. Defining it here gets the same access, which is
// what lets these tests seed the real store instead of substituting a fake
// for the read.
class HistoryEmbeddingsServicePublic : public HistoryEmbeddingsService {
 public:
  using HistoryEmbeddingsService::HistoryEmbeddingsService;
  using HistoryEmbeddingsService::OnPassagesEmbeddingsComputed;
  using HistoryEmbeddingsService::storage_;
};

// Exercises `GetPassagesForUrls` against a real service and its real store, so
// the URLID resolution, per-URL cap and truncation all run over data that
// actually round-tripped through storage.
class OpenTabPassagesTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(history_dir_.CreateUniqueTempDir());
    history_service_ = history::CreateHistoryService(history_dir_.GetPath(),
                                                     /*create_db=*/true);
    ASSERT_TRUE(history_service_);

    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);
    optimization_guide_model_provider_ = std::make_unique<
        optimization_guide::TestOptimizationGuideModelProvider>();
    page_content_annotations_service_ =
        page_content_annotations::TestPageContentAnnotationsService::Create(
            optimization_guide_model_provider_.get(), history_service_.get());
    ASSERT_TRUE(page_content_annotations_service_);
    page_content_extraction_service_ = std::make_unique<
        page_content_annotations::PageContentExtractionService>(
        nullptr, base::FilePath(), &mock_tracker_);
    page_embeddings_service_ =
        std::make_unique<page_content_annotations::PageEmbeddingsService>(
            page_content_extraction_service_.get());

    service_ = std::make_unique<HistoryEmbeddingsServicePublic>(
        os_crypt_.get(), history_service_.get(),
        page_content_annotations_service_.get(),
        /*optimization_guide_decider=*/nullptr, page_embeddings_service_.get(),
        passage_embeddings_test_env_.embedder_metadata_provider(),
        passage_embeddings_test_env_.embedder(),
        std::make_unique<MockAnswerer>(),
        std::make_unique<MockIntentClassifier>());
  }

  void TearDown() override {
    if (service_) {
      service_->storage_.SynchronouslyResetForTest();
      service_->Shutdown();
      service_.reset();
    }
  }

  // Seeds `passages` for the page added at `url_id`. History hands out URLIDs
  // in insertion order, starting at 1.
  void SeedPassages(const std::string& url,
                    history::URLID url_id,
                    const std::vector<std::string>& passages) {
    AddToHistory(url);
    std::vector<passage_embeddings::Embedding> embeddings(
        passages.size(), passage_embeddings::Embedding({1.0f, 0.0f, 0.0f}));
    UrlData url_data(url_id, /*visit_id=*/1, base::Time::Now());
    url_data.passages.mutable_passages()->Assign(passages.begin(),
                                                 passages.end());
    // Default-constructs each entry to nullopt.
    url_data.passage_embeddings.resize(passages.size());
    service_->OnPassagesEmbeddingsComputed(
        std::move(url_data), passages, std::move(embeddings), /*job_id=*/0,
        passage_embeddings::ComputeEmbeddingsStatus::kSuccess);
  }

  void AddToHistory(const std::string& url) {
    history_service_->AddPage(GURL(url), base::Time::Now(),
                              history::SOURCE_BROWSED);
  }

  std::vector<std::vector<std::string>> GetPassages(
      const std::vector<GURL>& urls) {
    base::test::TestFuture<std::vector<std::vector<std::string>>> future;
    GetPassagesForUrls(history_service_.get(), service_->AsWeakPtr(), urls,
                       kMaxPassagesPerUrl, kMaxPassageBytes,
                       future.GetCallback(), &task_tracker_);
    return future.Take();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir history_dir_;
  std::unique_ptr<history::HistoryService> history_service_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  std::unique_ptr<optimization_guide::TestOptimizationGuideModelProvider>
      optimization_guide_model_provider_;
  std::unique_ptr<page_content_annotations::PageContentAnnotationsService>
      page_content_annotations_service_;
  feature_engagement::test::MockTracker mock_tracker_;
  std::unique_ptr<page_content_annotations::PageContentExtractionService>
      page_content_extraction_service_;
  std::unique_ptr<page_content_annotations::PageEmbeddingsService>
      page_embeddings_service_;
  passage_embeddings::TestEnvironment passage_embeddings_test_env_;
  std::unique_ptr<HistoryEmbeddingsServicePublic> service_;
  base::CancelableTaskTracker task_tracker_;
};

TEST_F(OpenTabPassagesTest, NoUrls) {
  base::test::TestFuture<std::vector<std::vector<std::string>>> future;
  GetPassagesForUrls(history_service_.get(), service_->AsWeakPtr(), {},
                     kMaxPassagesPerUrl, kMaxPassageBytes, future.GetCallback(),
                     &task_tracker_);

  // Nothing to read, but the callback still has to be asynchronous so callers
  // see one contract on every path.
  EXPECT_FALSE(future.IsReady());
  EXPECT_TRUE(future.Take().empty());
}

TEST_F(OpenTabPassagesTest, PassagesForKnownUrls) {
  SeedPassages("http://test1.com", 1, {"hello"});
  SeedPassages("http://test2.com", 2, {"hello"});

  EXPECT_EQ(GetPassages({GURL("http://test1.com"), GURL("http://test2.com")}),
            (std::vector<std::vector<std::string>>{{"hello"}, {"hello"}}));
}

TEST_F(OpenTabPassagesTest, UnknownUrlKeepsItsSlot) {
  SeedPassages("http://test1.com", 1, {"hello"});

  // The unknown URL resolves to URLID 0, so it is never read, but it still
  // occupies its position so callers can zip the result with their own list.
  EXPECT_EQ(
      GetPassages({GURL("http://never-visited.com"), GURL("http://test1.com")}),
      (std::vector<std::vector<std::string>>{{}, {"hello"}}));
}

TEST_F(OpenTabPassagesTest, AllUrlsUnknownToHistory) {
  // None of the URLs resolve, so the lookup returns before any passage read.
  EXPECT_EQ(GetPassages({GURL("http://a.com"), GURL("http://b.com")}),
            (std::vector<std::vector<std::string>>{{}, {}}));
}

TEST_F(OpenTabPassagesTest, NoIndexedData) {
  AddToHistory("http://test1.com");

  // In history, but nothing was ever indexed for it.
  EXPECT_EQ(GetPassages({GURL("http://test1.com")}),
            (std::vector<std::vector<std::string>>{{}}));
}

TEST_F(OpenTabPassagesTest, CapsAndTruncatesPassages) {
  const std::string kept(kMaxPassageBytes, 'a');
  SeedPassages("http://test1.com", 1, {kept + "cut", "second", "third"});

  // Only `kMaxPassagesPerUrl` survive, and the first is cut to
  // `kMaxPassageBytes`.
  EXPECT_EQ(GetPassages({GURL("http://test1.com")}),
            (std::vector<std::vector<std::string>>{{kept, "second"}}));
}

TEST_F(OpenTabPassagesTest, CapAppliesPerUrl) {
  SeedPassages("http://test1.com", 1, {"one", "two", "three"});
  SeedPassages("http://test2.com", 2, {"four", "five", "six"});

  // `kMaxPassagesPerUrl` is a per-URL cap, not a budget for the request.
  EXPECT_EQ(GetPassages({GURL("http://test1.com"), GURL("http://test2.com")}),
            (std::vector<std::vector<std::string>>{{"one", "two"},
                                                   {"four", "five"}}));
}

TEST_F(OpenTabPassagesTest, EmptyPassagesSkippedWithoutConsumingCap) {
  SeedPassages("http://test1.com", 1, {"", "first", "second"});

  // The empty passage is dropped and doesn't count against the cap of 2.
  EXPECT_EQ(GetPassages({GURL("http://test1.com")}),
            (std::vector<std::vector<std::string>>{{"first", "second"}}));
}

TEST_F(OpenTabPassagesTest, TruncationPreservesUtf8Boundaries) {
  SeedPassages("http://test1.com", 1, {"日本語日本語"});

  // Six 3-byte characters: the 16-byte cap lands mid-character, so only five
  // whole characters may be kept.
  EXPECT_EQ(GetPassages({GURL("http://test1.com")}),
            (std::vector<std::vector<std::string>>{{"日本語日本"}}));
}

TEST_F(OpenTabPassagesTest, DuplicateUrlsEachGetPassages) {
  SeedPassages("http://test1.com", 1, {"hello"});

  // Several tabs on the same page each get the passages; the read is keyed by
  // URLID so the duplicates collapse into one.
  const GURL url("http://test1.com");
  EXPECT_EQ(
      GetPassages({url, url, url}),
      (std::vector<std::vector<std::string>>{{"hello"}, {"hello"}, {"hello"}}));
}

TEST_F(OpenTabPassagesTest, ResultsAlignToInputOrder) {
  SeedPassages("http://test1.com", 1, {"first"});
  SeedPassages("http://test2.com", 2, {"second"});
  SeedPassages("http://test3.com", 3, {"third"});

  // Reads complete on the storage sequence in an order this test doesn't
  // control, so each list must land on its own URL's slot.
  EXPECT_EQ(GetPassages({GURL("http://test3.com"), GURL("http://test1.com"),
                         GURL("http://test2.com")}),
            (std::vector<std::vector<std::string>>{
                {"third"}, {"first"}, {"second"}}));
}

TEST_F(OpenTabPassagesTest, HistoryUnavailable) {
  // A history service with no database cannot resolve URLIDs at all.
  base::ScopedTempDir no_db_dir;
  ASSERT_TRUE(no_db_dir.CreateUniqueTempDir());
  auto no_db_history =
      history::CreateHistoryService(no_db_dir.GetPath(), /*create_db=*/false);

  base::test::TestFuture<std::vector<std::vector<std::string>>> future;
  GetPassagesForUrls(no_db_history.get(), service_->AsWeakPtr(),
                     {GURL("http://test1.com")}, kMaxPassagesPerUrl,
                     kMaxPassageBytes, future.GetCallback(), &task_tracker_);

  EXPECT_EQ(future.Take(), (std::vector<std::vector<std::string>>{{}}));
}

TEST_F(OpenTabPassagesTest, ServiceShutDownDuringUrlLookup) {
  SeedPassages("http://test1.com", 1, {"hello"});

  base::test::TestFuture<std::vector<std::vector<std::string>>> future;
  GetPassagesForUrls(history_service_.get(), service_->AsWeakPtr(),
                     {GURL("http://test1.com")}, kMaxPassagesPerUrl,
                     kMaxPassageBytes, future.GetCallback(), &task_tracker_);

  // The URL lookup is still in flight. Shutdown releases the storage the read
  // would go to, so the hop must not dereference the service afterwards, and
  // the caller still has to hear back so its own callback isn't stranded.
  service_->Shutdown();

  EXPECT_EQ(future.Take(), (std::vector<std::vector<std::string>>{{}}));
}

}  // namespace history_embeddings
