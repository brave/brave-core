/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/components/sync/model/metadata_batch.cc"

namespace syncer {

void MetadataBatch::ClearProgressToken() {
  state_.mutable_progress_marker()->clear_token();
}

}  // namespace syncer
