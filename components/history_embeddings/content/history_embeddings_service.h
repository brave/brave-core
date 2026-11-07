// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_HISTORY_EMBEDDINGS_SERVICE_H_
#define BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_HISTORY_EMBEDDINGS_SERVICE_H_

#include <atomic>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/threading/sequence_bound.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "brave/components/history_embeddings/core/history_embeddings_search.h"
#include "brave/components/history_embeddings/core/sql_database.h"
#include "brave/components/history_embeddings/core/vector_database.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_service_observer.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/url_database.h"
#include "components/history/core/browser/url_row.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/page_content_annotations/content/page_embeddings_service.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"

namespace page_content_annotations {
class PageEmbeddingsService;
}  // namespace page_content_annotations

namespace os_crypt_async {
class OSCryptAsync;
}

namespace brave_history_embeddings {

using UrlDataCallback = base::OnceCallback<void(std::optional<UrlData>)>;

using PassagesStoredCallback = base::RepeatingCallback<void(UrlData)>;

class HistoryEmbeddingsService
    : public KeyedService,
      public HistoryEmbeddingsSearch,
      public history::HistoryServiceObserver,
      public passage_embeddings::EmbedderMetadataObserver,
      public page_content_annotations::PageEmbeddingsService::Observer {
 public:
  struct VisitMetadata {
    history::URLID url_id;
    history::VisitID visit_id;
    base::Time visit_time;
  };

  // Number of low-order bits to use in session_id for sequence number.
  static constexpr uint64_t kSessionIdSequenceBits = 16;
  static constexpr uint64_t kSessionIdSequenceBitMask =
      (1 << kSessionIdSequenceBits) - 1;

  // `history_service` is never nullptr and must outlive `this`.
  // Storage uses its `history_dir() location for the database.
  HistoryEmbeddingsService(
      os_crypt_async::OSCryptAsync* os_crypt_async,
      history::HistoryService* history_service,
      page_content_annotations::PageEmbeddingsService* page_embeddings_service,
      passage_embeddings::EmbedderMetadataProvider* embedder_metadata_provider,
      passage_embeddings::Embedder* embedder);
  HistoryEmbeddingsService(const HistoryEmbeddingsService&) = delete;
  HistoryEmbeddingsService& operator=(const HistoryEmbeddingsService&) = delete;
  ~HistoryEmbeddingsService() override;

  // Updates the current history visit metadata for the current navigation in
  // the WebContents. std::nullopt should be passed if the navigation does not
  // have a history visit.
  void UpdateVisitMetadata(content::WebContents* web_contents,
                           const std::optional<VisitMetadata>& visit_metadata);

  // HistoryEmbeddingsSearch:
  SearchResult Search(SearchResult* previous_search_result,
                      std::string query,
                      std::optional<base::Time> time_range_start,
                      size_t count,
                      std::vector<history::URLID> url_id_filter,
                      SearchResultCallback callback) override;

  // Weak `this` provider method.
  base::WeakPtr<HistoryEmbeddingsService> AsWeakPtr();

  // KeyedService:
  void Shutdown() override;

  // history::HistoryServiceObserver:
  void OnHistoryDeletions(history::HistoryService* history_service,
                          const history::DeletionInfo& deletion_info) override;

  // page_content_annotations::PageEmbeddingsService::Observer:
  page_content_annotations::PageEmbeddingsService::Priority GetDefaultPriority()
      const override;
  page_content_annotations::PageEmbeddingsService::UsageMode GetUsageMode()
      const override;
  void OnPageEmbeddingsAvailable(content::Page& page) override;

  // Asynchronously gets passages and embeddings from storage for given
  // `url_id`. Calls `callback` with the data or nullopt if no data is found in
  // the HistoryEmbeddings database.
  void GetUrlData(history::URLID url_id, UrlDataCallback callback) const;

  // Asynchronously gets passages and embeddings from storage where visits
  // are within a given time range. Calls `callback` with the data.
  // The `limit` and `offset` can be used to control data range with
  // standard SQL style paging.
  void GetUrlDataInTimeRange(
      base::Time from_time,
      base::Time to_time,
      size_t limit,
      size_t offset,
      base::OnceCallback<void(std::vector<UrlData>)> callback) const;

  // Set a callback to be called when `ProcessAndStorePassages` completes.
  void SetPassagesStoredCallbackForTesting(PassagesStoredCallback callback);

 private:
  friend class HistoryEmbeddingsServicePublic;

  // A utility container to wrap anything that should be accessed on
  // the separate storage worker sequence.
  struct Storage {
    Storage(const base::FilePath& storage_dir,
            bool erase_non_ascii_characters,
            bool delete_embeddings);

    // Associate the given metadata with this Storage instance. The storage is
    // not considered initialized until this metadata is supplied.
    void SetEmbedderMetadata(
        passage_embeddings::EmbedderMetadata metadata,
        scoped_refptr<os_crypt_async::Encryptor> encryptor);

    // Called on the worker sequence to persist passages and embeddings.
    void ProcessAndStorePassages(UrlData url_data);

    // Runs search on worker sequence.
    std::vector<ScoredUrlRow> Search(
        base::WeakPtr<std::atomic<size_t>> weak_latest_query_id,
        size_t query_id,
        SearchParams search_params,
        passage_embeddings::Embedding query_embedding,
        std::optional<base::Time> time_range_start,
        size_t count);

    // Handles the History deletions on the worker thread.
    void HandleHistoryDeletions(bool for_all_history,
                                history::URLRows deleted_rows,
                                std::set<history::VisitID> deleted_visit_ids);

    // Gathers URL and passage data from the database where corresponding
    // embeddings are absent. This is used to rebuild the embeddings table
    // when the model changes.
    std::vector<UrlData> CollectPassagesWithoutEmbeddings();

    // Retrieves passages and embeddings from the database for use as a cache
    // to avoid recomputing embeddings that exist for identical passages.
    std::optional<UrlData> GetUrlData(history::URLID url_id);

    // Retrieves passages and embeddings from the database that have visit times
    // within specified range.
    std::vector<UrlData> GetUrlDataInTimeRange(base::Time from_time,
                                               base::Time to_time,
                                               size_t limit,
                                               size_t offset);

    // A VectorDatabase implementation that holds data in memory.
    VectorDatabaseInMemory vector_database;

    // The underlying SQL database for persistent storage.
    SqlDatabase sql_database;
  };

  // passage_embeddings::EmbedderMetadataObserver:
  // Passes the metadata to the internal storage.
  void EmbedderMetadataUpdated(
      passage_embeddings::EmbedderMetadata metadata) override;

  void OnOsCryptAsyncReady(scoped_refptr<os_crypt_async::Encryptor> encryptor);

  // Stores the passages and embeddings for the URL in the database.
  void StorePassageEmbeddings(
      history::URLID url_id,
      history::VisitID visit_id,
      base::Time visit_time,
      std::vector<page_content_annotations::PassageEmbedding>
          passage_embeddings);

  // Invoked after the embeddings for `passages` has been computed. Stores the
  // passages along with their embeddings in the database.
  void OnPassagesEmbeddingsComputed(
      UrlData url_passages,
      std::vector<std::string> passages,
      std::vector<passage_embeddings::Embedding> embeddings,
      uint64_t job_id,
      passage_embeddings::ComputeEmbeddingsStatus status);

  // Invoked after the embedding for the original search query has been
  // computed.
  void OnQueryEmbeddingComputed(
      SearchResultCallback callback,
      SearchResult result,
      std::vector<std::string> query_passages,
      std::vector<passage_embeddings::Embedding> query_embedding,
      uint64_t job_id,
      passage_embeddings::ComputeEmbeddingsStatus status);

  // Finishes a search result by combining found data with additional data from
  // history database. Moves each ScoredUrl into a more complete structure with
  // a history URLRow. Omits any entries that don't have corresponding data in
  // the history database.
  void OnSearchCompleted(SearchResultCallback callback,
                         SearchResult result,
                         std::vector<ScoredUrlRow> scored_url_rows);

  // Rebuild absent embeddings from source passages.
  void RebuildAbsentEmbeddings(std::vector<UrlData> all_url_passages);

  // Returns true if query should be filtered. If false, then `search_params`
  // will have its query_terms set.
  bool QueryIsFiltered(const std::string& raw_query,
                       SearchParams& search_params) const;

  raw_ptr<os_crypt_async::OSCryptAsync> os_crypt_async_;

  // The history service is used to fill in details about URLs and visits
  // found via search. It strictly outlives this due to the dependency
  // specified in HistoryEmbeddingsServiceFactory.
  raw_ptr<history::HistoryService> history_service_;

  raw_ptr<page_content_annotations::PageEmbeddingsService>
      page_embeddings_service_;

  // Tracks the observed history service, for cleanup.
  base::ScopedObservation<history::HistoryService,
                          history::HistoryServiceObserver>
      history_service_observation_{this};

  // The embedder used to compute embeddings. Outlives this.
  raw_ptr<passage_embeddings::Embedder> embedder_;

  // If the current navigation in the WebContents corresponds to a history
  // visit, holds information about the visit.
  base::flat_map<content::WebContents*, VisitMetadata> last_history_visit_;

  // Metadata about the embedder; Set when valid metadata is received from
  // `embedder_metadata_provider`.
  passage_embeddings::EmbedderMetadata embedder_metadata_{0, 0};

  // Storage is bound to a separate sequence.
  // This will be null if the feature flag is disabled.
  base::SequenceBound<Storage> storage_;

  // Callback called when `ProcessAndStorePassages` completes. Needed for tests
  // as the blink dependency doesn't have a 'wait for pending requests to
  // complete' mechanism.
  PassagesStoredCallback passages_stored_callback_for_tests_ =
      base::DoNothing();

  // A thread-safe invalidation mechanism to halt searches for stale queries:
  // Each query is run with the current `query_id_` and a weak pointer to the
  // atomic value itself. When it changes, any queries other than the latest
  // can be halted. Note this is not job cancellation, it breaks the inner
  // search loop while running so the atomic is needed for thread safety.
  std::atomic<size_t> query_id_ = 0u;

  // A list of in-flight jobs for rebuilding absent embeddings.
  base::flat_set<passage_embeddings::Embedder::Job,
                 passage_embeddings::Embedder::JobIdComparator>
      rebuild_jobs_;

  // Used to cancel the in-flight embedding job for the previous stale query.
  std::optional<passage_embeddings::Embedder::Job> query_embedding_job_;

  base::ScopedObservation<
      page_content_annotations::PageEmbeddingsService,
      page_content_annotations::PageEmbeddingsService::Observer>
      page_embeddings_observation_{this};

  // Scoped observation for when the embedder metadata is available.
  base::ScopedObservation<passage_embeddings::EmbedderMetadataProvider,
                          passage_embeddings::EmbedderMetadataObserver>
      embedder_metadata_observation_{this};

  base::WeakPtrFactory<std::atomic<size_t>> query_id_weak_ptr_factory_;

  base::WeakPtrFactory<HistoryEmbeddingsService> weak_ptr_factory_;
};

}  // namespace brave_history_embeddings

#endif  // BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_HISTORY_EMBEDDINGS_SERVICE_H_
