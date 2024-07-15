#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BACKGROUND_HISTORY_QUERIER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BACKGROUND_HISTORY_QUERIER_H_

#include "base/functional/callback_forward.h"
#include "components/history/core/browser/history_service.h"

namespace brave_news {

using QueryHistoryCallback = history::HistoryService::QueryHistoryCallback;
using BackgroundHistoryQuerier =
    base::RepeatingCallback<void(QueryHistoryCallback)>;

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BACKGROUND_HISTORY_QUERIER_H_
