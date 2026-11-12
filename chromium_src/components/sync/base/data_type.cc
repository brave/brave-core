/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define EncryptableUserTypes EncryptableUserTypes_ChromiumImpl
#define LowPriorityUserTypes LowPriorityUserTypes_ChromiumImpl

#include <components/sync/base/data_type.cc>

#undef LowPriorityUserTypes
#undef EncryptableUserTypes

namespace syncer {

DataTypeSet EncryptableUserTypes() {
  // Brave's sync chain is always in encrypt-everything mode, so subtract from
  // every user type instead of following upstream's EncryptionPolicy. Its
  // kNeverEncrypted rationales are written for a server that reads and writes
  // the data, which Brave's never does, so a type upstream adds or switches to
  // kNeverEncrypted stays encrypted here by default.
  DataTypeSet encryptable_user_types =
      base::Difference(UserTypes(), CommitOnlyTypes());
  // Meant to be read by the server, which holds no key material.
  // https://github.com/brave/go-sync/issues/178
  encryptable_user_types.Remove(HISTORY_DELETE_DIRECTIVES);
  return encryptable_user_types;
}

DataTypeSet LowPriorityUserTypes() {
  auto low_priority_user_types = LowPriorityUserTypes_ChromiumImpl();
  // Directives must be synced after history entities. If
  // history delete directives are processed before retrieving history upon
  // initial sync, relevant entries will not be deleted.
  // This override must be reverted when
  // https://github.com/brave/go-sync/issues/178 will be solved.
  low_priority_user_types.Remove(HISTORY);
  low_priority_user_types.Put(HISTORY_DELETE_DIRECTIVES);

  return low_priority_user_types;
}

}  // namespace syncer
