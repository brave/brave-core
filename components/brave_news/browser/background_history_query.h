#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BACKGROUND_HISTORY_QUERY_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BACKGROUND_HISTORY_QUERY_H_

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/history/core/browser/history_service.h"
namespace brave_news {

using QueryHistoryCallback = history::HistoryService::QueryHistoryCallback;
using BackgroundHistoryQuerier =
    base::RepeatingCallback<void(QueryHistoryCallback)>;

// Creates a function for querying history from a non-main thread. This lets us
// lazily pull a recent snapshot of history into our worker task.
BackgroundHistoryQuerier MakeHistoryQuerier(
    base::WeakPtr<history::HistoryService> history_service,
    base::CancelableTaskTracker* task_tracker);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BACKGROUND_HISTORY_QUERY_H_
