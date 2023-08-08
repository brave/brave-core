/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/sync/service/glue/sync_engine_impl.cc"

#include "base/task/bind_post_task.h"

namespace syncer {

void SyncEngineImpl::PermanentlyDeleteAccount(
    base::OnceCallback<void(const SyncProtocolError&)> callback) {
  DCHECK(backend_);
  sync_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&SyncEngineBackend::PermanentlyDeleteAccount, backend_,
                     base::BindPostTaskToCurrentDefault(std::move(callback))));
}

}  // namespace syncer
