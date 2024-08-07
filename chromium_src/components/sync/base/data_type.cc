/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define EncryptableUserTypes EncryptableUserTypes_ChromiumImpl
#include "src/components/sync/base/data_type.cc"
#undef EncryptableUserTypes

namespace syncer {

DataTypeSet EncryptableUserTypes() {
  DataTypeSet encryptable_user_types = EncryptableUserTypes_ChromiumImpl();
  // Brave sync has encryption setup ready when sync chain created
  encryptable_user_types.Put(DEVICE_INFO);
  encryptable_user_types.Put(HISTORY);
  return encryptable_user_types;
}

}  // namespace syncer
