/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_COMMIT_ENCRYPTION_CHECK_H_
#define BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_COMMIT_ENCRYPTION_CHECK_H_

#include "components/sync/base/data_type.h"

namespace sync_pb {
class CommitMessage;
}  // namespace sync_pb

namespace syncer {

// Returns the types of the entities in `commit` that carry unencrypted
// specifics and are not expected to.
DataTypeSet GetUnexpectedPlainTextCommitTypes(
    const sync_pb::CommitMessage& commit);

// Reports a crash dump when `commit` is about to leave the device carrying
// unencrypted specifics for a type Brave expects to be encrypted.
void CheckCommitEntitiesEncryption(const sync_pb::CommitMessage& commit);

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_COMMIT_ENCRYPTION_CHECK_H_
