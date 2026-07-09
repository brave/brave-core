// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_OPEN_TAB_SEARCH_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_OPEN_TAB_SEARCH_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"

namespace base {
class CancelableTaskTracker;
}  // namespace base

namespace history {
class HistoryService;
}  // namespace history

class Profile;

namespace history_embeddings {

class HistoryEmbeddingsSearch;

// Callback signature that matches the mojom-generated
// `TabSearchPageHandler::SearchTabsByContentCallback`: receives the tab_ids
// of matched open tabs in ranked order (best first).
using RankedTabIdsCallback =
    base::OnceCallback<void(const std::vector<int32_t>&)>;

// Snapshots `profile`'s open HTTP(S) tabs from regular-type windows tracked
// by the tab_search feature, resolves their URLs to URLIDs via
// `history_service`, ranks them against `query` with
// `HistoryEmbeddingsSearch::Search` (`skip_answering=true`), and dispatches
// the matched tab_ids (best first) to `callback`. Dispatches an empty list
// when there are no such tabs or none of them match.
void SearchOpenTabsByContent(Profile* profile,
                             history::HistoryService* history_service,
                             HistoryEmbeddingsSearch* embeddings_search,
                             std::string query,
                             RankedTabIdsCallback callback,
                             base::CancelableTaskTracker* task_tracker);

}  // namespace history_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_OPEN_TAB_SEARCH_H_
