// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/history_embeddings/content/open_tab_passages.h"

#include <optional>
#include <string_view>
#include <utility>

#include "base/barrier_callback.h"
#include "base/check_op.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history_embeddings/content/history_embeddings_service.h"
#include "components/history_embeddings/core/vector_database.h"
#include "url/gurl.h"

namespace history_embeddings {

namespace {

// A URL's passages tagged with every position it occupies in the caller's URL
// list, so the barrier's completion-ordered results can be put back in order.
// More than one position means several tabs are on the same page.
struct IndexedPassages {
  std::vector<size_t> indices;
  std::vector<std::string> passages;
};

std::vector<std::string> TakePassages(size_t max_passages_per_url,
                                      size_t max_passage_bytes,
                                      std::optional<UrlData> url_data) {
  std::vector<std::string> passages;
  if (!url_data) {
    return passages;
  }
  for (const std::string& passage : url_data->passages.passages()) {
    if (passages.size() >= max_passages_per_url) {
      break;
    }
    std::string_view truncated =
        base::TruncateUTF8ToByteSize(passage, max_passage_bytes);
    if (truncated.empty()) {
      continue;
    }
    passages.push_back(std::string(truncated));
  }
  return passages;
}

void OnUrlDataFetched(std::vector<size_t> indices,
                      size_t max_passages_per_url,
                      size_t max_passage_bytes,
                      base::OnceCallback<void(IndexedPassages)> barrier,
                      std::optional<UrlData> url_data) {
  std::move(barrier).Run(IndexedPassages{
      std::move(indices), TakePassages(max_passages_per_url, max_passage_bytes,
                                       std::move(url_data))});
}

void DispatchPassages(size_t url_count,
                      TabPassagesCallback callback,
                      std::vector<IndexedPassages> results) {
  std::vector<std::vector<std::string>> passages_by_url(url_count);
  for (auto& result : results) {
    for (size_t i = 0; i < result.indices.size(); ++i) {
      // Several tabs can share a URL, but the last one can take the passages
      // rather than copy them, which is the only case for most requests.
      passages_by_url[result.indices[i]] = i + 1 == result.indices.size()
                                               ? std::move(result.passages)
                                               : result.passages;
    }
  }
  std::move(callback).Run(std::move(passages_by_url));
}

void OnUrlIdsResolved(base::WeakPtr<HistoryEmbeddingsService> service,
                      size_t url_count,
                      size_t max_passages_per_url,
                      size_t max_passage_bytes,
                      TabPassagesCallback callback,
                      std::optional<std::vector<history::URLID>> url_ids) {
  // HistoryService returned no result (e.g. shutdown / cancellation), or the
  // embeddings service was shut down while the URL lookup was in flight.
  if (!url_ids || !service) {
    std::move(callback).Run(std::vector<std::vector<std::string>>(url_count));
    return;
  }
  CHECK_EQ(url_count, url_ids->size());

  // One read per distinct URLID. Several tabs on the same page are common,
  // and each read decrypts a blob on the embeddings storage sequence, so the
  // duplicates are worth collapsing before fanning out.
  base::flat_map<history::URLID, std::vector<size_t>> indices_by_url_id;
  for (size_t i = 0; i < url_ids->size(); ++i) {
    // History has never seen this URL, so it has no passages either.
    if ((*url_ids)[i] != 0) {
      indices_by_url_id[(*url_ids)[i]].push_back(i);
    }
  }
  if (indices_by_url_id.empty()) {
    std::move(callback).Run(std::vector<std::vector<std::string>>(url_count));
    return;
  }

  auto barrier = base::BarrierCallback<IndexedPassages>(
      indices_by_url_id.size(),
      base::BindOnce(&DispatchPassages, url_count, std::move(callback)));
  for (auto& [url_id, indices] : indices_by_url_id) {
    service->GetUrlData(
        url_id,
        base::BindOnce(&OnUrlDataFetched, std::move(indices),
                       max_passages_per_url, max_passage_bytes, barrier));
  }
}

}  // namespace

void GetPassagesForUrls(history::HistoryService* history_service,
                        base::WeakPtr<HistoryEmbeddingsService> service,
                        const std::vector<GURL>& urls,
                        size_t max_passages_per_url,
                        size_t max_passage_bytes,
                        TabPassagesCallback callback,
                        base::CancelableTaskTracker* task_tracker) {
  if (urls.empty()) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  std::vector<std::vector<std::string>>()));
    return;
  }
  history_service->QueryUrlIds(
      urls,
      base::BindOnce(&OnUrlIdsResolved, std::move(service), urls.size(),
                     max_passages_per_url, max_passage_bytes,
                     std::move(callback)),
      task_tracker);
}

}  // namespace history_embeddings
