/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/engine/brave_commit_encryption_check.h"

#include "base/check.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "components/sync/protocol/entity_specifics.pb.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/protocol/sync_entity.pb.h"

namespace syncer {

namespace {

// Brave's sync chain is always in encrypt-everything mode, so these are the
// only types whose entities may leave the device with `specifics.encrypted`
// unset.
constexpr DataTypeSet kPlainTextCommitAllowedTypes = {
    // Carries the key bag that protects every other type.
    NIGORI,
    // Meant to be read by the server, which holds no key material, so it
    // cannot be encrypted. go-sync does not apply the directives yet and only
    // stores and relays them. The payload is visit ids and a time range;
    // UrlDirective, the only variant carrying a URL, is stubbed out in Brave's
    // DeleteDirectiveHandler.
    HISTORY_DELETE_DIRECTIVES,
    // Encrypted by its own scheme into its own specifics fields, so
    // `specifics.encrypted` stays unset. See
    // DataTypeWorker::EncryptPasswordSpecificsData().
    PASSWORDS,
};

// OUTGOING_PASSWORD_SHARING_INVITATION is deliberately absent. It is encrypted
// the same way PASSWORDS is, but Brave can never produce an entity of it: the
// share flow needs a recipient list from Google's PasswordSharingRecipients
// endpoint. Should one ever be reported here, add it rather than assume a leak.

}  // namespace

DataTypeSet GetUnexpectedPlainTextCommitTypes(
    const sync_pb::CommitMessage& commit) {
  DataTypeSet unexpected_types;
  for (const sync_pb::SyncEntity& entity : commit.entries()) {
    // A deletion has empty specifics, so there is nothing to encrypt.
    if (entity.deleted() || entity.specifics().has_encrypted()) {
      continue;
    }
    const DataType type = GetDataTypeFromSpecifics(entity.specifics());
    // DataTypeSet cannot hold UNSPECIFIED, and a locally authored entity
    // always has a known specifics variant.
    if (type == UNSPECIFIED || kPlainTextCommitAllowedTypes.Has(type)) {
      continue;
    }
    unexpected_types.Put(type);
  }
  return unexpected_types;
}

void CheckCommitEntitiesEncryption(const sync_pb::CommitMessage& commit) {
  const DataTypeSet unexpected_types =
      GetUnexpectedPlainTextCommitTypes(commit);
  DCHECK(unexpected_types.empty())
      << "Committing unencrypted sync specifics for "
      << DataTypeSetToDebugString(unexpected_types);
  if (unexpected_types.empty()) {
    return;
  }
  // Release only: the DCHECK message is compiled out, so the crash key is the
  // only way the type names reach the dump.
  SCOPED_CRASH_KEY_STRING256("BraveSync", "plain_text_commit_types",
                             DataTypeSetToDebugString(unexpected_types));
  base::debug::DumpWithoutCrashing();
}

}  // namespace syncer
