/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/history/core/browser/history_backend.cc"

namespace history {

HistoryCountResult HistoryBackend::GetKnownToSyncCount() {
  int count = 0;
  return {db_ && db_->GetKnownToSyncCount(&count), count};
}

}  // namespace history
