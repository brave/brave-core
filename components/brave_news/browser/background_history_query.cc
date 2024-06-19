#include "brave/components/brave_news/browser/background_history_query.h"

#include <string>

#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/task/bind_post_task.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/history/core/browser/history_service.h"

namespace brave_news {

namespace {}

// TODO: |tracker| needs to live somewhere else - its not safe passing a raw_ptr
// through like this.
BackgroundHistoryQuerier MakeHistoryQuerier(
    base::WeakPtr<history::HistoryService> history_service,
    base::CancelableTaskTracker* tracker) {
  auto history_sequence = base::SequencedTaskRunner::GetCurrentDefault();
  return base::BindRepeating(
      [](scoped_refptr<base::SequencedTaskRunner> history_sequence,
         base::WeakPtr<history::HistoryService> history_service,
         base::CancelableTaskTracker* tracker, QueryHistoryCallback callback) {
        // |bound_callback| will always be invoked on the caller's thread.
        auto bound_callback =
            base::BindPostTaskToCurrentDefault(std::move(callback));

        history_sequence->PostTask(
            FROM_HERE,
            base::BindOnce(
                [](base::WeakPtr<history::HistoryService> service,
                   base::CancelableTaskTracker* tracker,
                   QueryHistoryCallback callback) {
                  if (!service) {
                    return;
                  }

                  history::QueryOptions options;
                  options.max_count = 2000;
                  options.SetRecentDayRange(14);
                  service->QueryHistory(std::u16string(), options,
                                        std::move(callback), tracker);
                },
                history_service, tracker, std::move(bound_callback)));
      },
      history_sequence, history_service, tracker);
}
}  // namespace brave_news
