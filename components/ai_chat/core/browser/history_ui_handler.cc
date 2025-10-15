// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/history_ui_handler.h"

#include "base/strings/utf_string_conversions.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace ai_chat {

namespace {
constexpr size_t kDefaultMaxResults = 100;
}

HistoryUIHandler::HistoryUIHandler(
    mojo::PendingReceiver<mojom::HistoryUIHandler> receiver,
    history::HistoryService* history_service)
    : receiver_(this, std::move(receiver)), history_service_(history_service) {}

HistoryUIHandler::~HistoryUIHandler() = default;

void HistoryUIHandler::GetHistory(const std::optional<std::string>& query,
                                  std::optional<uint32_t> max_results,
                                  GetHistoryCallback callback) {
  history::QueryOptions options;
  options.visit_order = history::QueryOptions::RECENT_FIRST;
  options.max_count = max_results.value_or(kDefaultMaxResults);

  // Note: `HistoryService::QueryHistory` returns no results if the query is
  // less than 2 characters, so no point passing it to the history service.
  auto query_string = query.value_or("");
  if (query_string.length() <= 2) {
    query_string = "";
  }

  history_service_->QueryHistory(
      base::UTF8ToUTF16(query_string), std::move(options),
      base::BindOnce(&HistoryUIHandler::OnGetHistory,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)),
      &task_tracker_);
}

void HistoryUIHandler::OnGetHistory(GetHistoryCallback callback,
                                    history::QueryResults results) {
  std::vector<mojom::HistoryEntryPtr> history_entries;
  for (const auto& result : results) {
    history_entries.push_back(mojom::HistoryEntry::New(
        result.id(), base::UTF16ToUTF8(result.title()), result.url()));
  }
  std::move(callback).Run(std::move(history_entries));
}

}  // namespace ai_chat
