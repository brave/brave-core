/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_COMMIT_ENCRYPTION_CHECK_H_
#define BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_COMMIT_ENCRYPTION_CHECK_H_

#include <stddef.h>

#include "components/sync/base/data_type.h"
#include "components/sync/base/passphrase_enums.h"
#include "components/sync/engine/commit_and_get_updates_types.h"

namespace syncer {

struct EntityData;

// Returns whether `entity` of `type` carries unencrypted specifics for a type
// Brave expects to be encrypted.
bool IsUnexpectedPlainTextCommit(DataType type, const EntityData& entity);

// Drops from `commit_requests` every entity that would otherwise leave the
// device carrying unencrypted specifics for a type Brave expects to be
// encrypted, and returns how many were dropped. Must be called after the worker
// has encrypted the list. Does nothing unless `passphrase_type` is an explicit
// one, which is what puts the chain in encrypt-everything mode. Reporting is
// the caller's job, via ReportDroppedUnencryptedCommits(), so that the drop
// itself stays testable.
size_t DropUnencryptedCommitRequests(DataType type,
                                     PassphraseType passphrase_type,
                                     CommitRequestDataList* commit_requests);

// Dumps for entities of `type` dropped by DropUnencryptedCommitRequests().
void ReportDroppedUnencryptedCommits(DataType type);

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_COMMIT_ENCRYPTION_CHECK_H_
