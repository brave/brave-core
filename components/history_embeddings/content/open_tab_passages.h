// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_OPEN_TAB_PASSAGES_H_
#define BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_OPEN_TAB_PASSAGES_H_

#include <cstddef>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"

class GURL;

namespace base {
class CancelableTaskTracker;
}  // namespace base

namespace history {
class HistoryService;
}  // namespace history

namespace history_embeddings {

class HistoryEmbeddingsService;

// Receives one passage list per requested URL, in the order the URLs were
// passed in. A URL with no indexed content gets an empty list, so the result
// is always the same size as the input.
using TabPassagesCallback =
    base::OnceCallback<void(std::vector<std::vector<std::string>>)>;

// Resolves `urls` to URLIDs via `history_service` and collects each one's
// stored passages from `service`, keeping at most
// `max_passages_per_url` passages per URL, each truncated to
// `max_passage_bytes`.
//
// URLs that history doesn't know, and pages the embeddings database has no
// data for, yield an empty list rather than dropping out of the result.
//
// `service` is weak because the URL lookup is asynchronous and the service is
// a KeyedService whose storage is released in Shutdown(); reading it
// afterwards is a contract violation. An invalidated pointer yields empty
// lists, so `callback` always runs.
void GetPassagesForUrls(history::HistoryService* history_service,
                        base::WeakPtr<HistoryEmbeddingsService> service,
                        const std::vector<GURL>& urls,
                        size_t max_passages_per_url,
                        size_t max_passage_bytes,
                        TabPassagesCallback callback,
                        base::CancelableTaskTracker* task_tracker);

}  // namespace history_embeddings

#endif  // BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_OPEN_TAB_PASSAGES_H_
