/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/sync/service/glue/sync_engine_backend.cc"

#include "components/sync/engine/sync_protocol_error.h"

namespace syncer {

void SyncEngineBackend::PermanentlyDeleteAccount(
    base::OnceCallback<void(const SyncProtocolError&)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  sync_manager_->PermanentlyDeleteAccount(std::move(callback));
}

}  // namespace syncer
