/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/engine/brave_commit_encryption_check.h"

#include <vector>

#include "base/notreached.h"
#include "components/sync/protocol/entity_data.h"
#include "components/sync/protocol/entity_specifics.pb.h"

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
    // stores and relays them; https://github.com/brave/go-sync/issues/178
    // tracks it. The payload is visit ids and a time range; UrlDirective, the
    // only variant carrying a URL, is stubbed out in Brave's
    // DeleteDirectiveHandler, at chromium_src/.../delete_directive_handler.cc.
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

bool IsUnexpectedPlainTextCommit(DataType type, const EntityData& entity) {
  // A deletion carries no specifics, so there is nothing to encrypt.
  if (entity.is_deleted() || entity.specifics.has_encrypted()) {
    return false;
  }
  return !kPlainTextCommitAllowedTypes.Has(type);
}

size_t DropUnencryptedCommitRequests(DataType type,
                                     PassphraseType passphrase_type,
                                     CommitRequestDataList* commit_requests) {
  // Without an explicit passphrase the chain is not in encrypt-everything
  // mode, and a type whose EncryptionPolicy is kNeverEncrypted is expected to
  // commit in plain text.
  if (!IsExplicitPassphrase(passphrase_type)) {
    return 0;
  }
  return std::erase_if(*commit_requests, [type](const auto& commit_request) {
    return IsUnexpectedPlainTextCommit(type, *commit_request->entity);
  });
}

void ReportDroppedUnencryptedCommits(DataType type) {
  DUMP_WILL_BE_NOTREACHED()
      << "Dropped a commit of unencrypted sync specifics for "
      << DataTypeToDebugString(type);
}

}  // namespace syncer
