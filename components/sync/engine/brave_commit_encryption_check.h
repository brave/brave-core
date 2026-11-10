/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_COMMIT_ENCRYPTION_CHECK_H_
#define BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_COMMIT_ENCRYPTION_CHECK_H_

#include "components/sync/base/data_type.h"
#include "components/sync/base/passphrase_enums.h"

namespace sync_pb {
class SyncEntity;
}  // namespace sync_pb

namespace syncer {

// Returns whether `entity` of `type` carries unencrypted specifics for a type
// Brave expects to be encrypted.
bool IsUnexpectedPlainTextCommit(DataType type,
                                 const sync_pb::SyncEntity& entity);

// Reports a crash dump when `entity` is about to leave the device carrying
// unencrypted specifics for a type Brave expects to be encrypted. Does nothing
// unless `passphrase_type` is an explicit one, which is what puts the chain in
// encrypt-everything mode.
void CheckCommitEntityEncryption(DataType type,
                                 PassphraseType passphrase_type,
                                 const sync_pb::SyncEntity& entity);

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_COMMIT_ENCRYPTION_CHECK_H_
