// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_HISTORY_UI_HANDLER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_HISTORY_UI_HANDLER_H_

#include "base/task/cancelable_task_tracker.h"
#include "brave/components/ai_chat/core/common/mojom/history.mojom.h"
#include "components/history/core/browser/history_types.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace history {
class HistoryService;
}  // namespace history

namespace ai_chat {
class HistoryUIHandler : public mojom::HistoryUIHandler {
 public:
  explicit HistoryUIHandler(
      mojo::PendingReceiver<mojom::HistoryUIHandler> receiver,
      history::HistoryService* history_service);
  ~HistoryUIHandler() override;

  void GetHistory(const std::optional<std::string>& query,
                  std::optional<uint32_t> max_results,
                  GetHistoryCallback callback) override;

 private:
  void OnGetHistory(GetHistoryCallback callback, history::QueryResults results);

  mojo::Receiver<mojom::HistoryUIHandler> receiver_;
  raw_ptr<history::HistoryService> history_service_;
  base::CancelableTaskTracker task_tracker_;
  base::WeakPtrFactory<HistoryUIHandler> weak_ptr_factory_{this};
};
}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_HISTORY_UI_HANDLER_H_
