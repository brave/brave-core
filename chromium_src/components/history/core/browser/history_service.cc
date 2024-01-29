/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/history/core/browser/history_service.cc"

#include "base/task/bind_post_task.h"

namespace history {

void HistoryService::GetKnownToSyncCount(
    base::OnceCallback<void(HistoryCountResult)> callback) {
  backend_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&HistoryBackend::GetKnownToSyncCount, history_backend_),
      base::BindPostTaskToCurrentDefault(std::move(callback)));
}

}  // namespace history
