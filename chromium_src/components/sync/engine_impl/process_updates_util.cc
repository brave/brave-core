/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// TODO(darkdh): investigate the records flow causing object id not applied to
// local entry in CommitResponse

#define BRAVE_PROCESS_UPDATE                                   \
  DCHECK(false) << "Brave object id " << server_id             \
                << " is not applied to local id " << local_id; \
  return;

#include "../../../../../components/sync/engine_impl/process_updates_util.cc"  // NOLINT
#undef BRAVE_PROCESS_UPDATE
