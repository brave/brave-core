// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/background_history_querier.h"

#include <utility>

#include "base/check.h"
#include "base/task/bind_post_task.h"
#include "components/history/core/browser/history_types.h"
#include "content/public/browser/browser_thread.h"

namespace brave_news {

BackgroundHistoryQuerier MakeHistoryQuerier(
    base::WeakPtr<history::HistoryService> history_service,
    base::RepeatingCallback<base::CancelableTaskTracker*()> get_tracker) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto history_sequence = base::SequencedTaskRunner::GetCurrentDefault();
  return base::BindRepeating(
      [](scoped_refptr<base::SequencedTaskRunner> history_sequence,
         base::WeakPtr<history::HistoryService> history_service,
         base::RepeatingCallback<base::CancelableTaskTracker*()> get_tracker,
         QueryHistoryCallback callback) {
        // |bound_callback| will always be invoked on the caller's thread.
        auto bound_callback =
            base::BindPostTaskToCurrentDefault(std::move(callback));

        history_sequence->PostTask(
            FROM_HERE,
            base::BindOnce(
                [](base::WeakPtr<history::HistoryService> service,
                   base::RepeatingCallback<base::CancelableTaskTracker*()>
                       get_tracker,
                   QueryHistoryCallback callback) {
                  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
                  auto* tracker = get_tracker.Run();
                  if (!service || !tracker) {
                    std::move(callback).Run(history::QueryResults());
                    return;
                  }

                  history::QueryOptions options;
                  options.max_count = 2000;
                  options.SetRecentDayRange(14);
                  service->QueryHistory(std::u16string(), options,
                                        std::move(callback), tracker);
                },
                history_service, get_tracker, std::move(bound_callback)));
      },
      history_sequence, history_service, get_tracker);
}

}  // namespace brave_news
