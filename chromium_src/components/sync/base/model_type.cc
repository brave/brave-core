/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync/base/model_type.h"

#include "base/feature_list.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/sync/protocol/entity_specifics.pb.h"

#define kHistory                                               \
  kHistory                                                     \
  }                                                            \
  ,                                                            \
      {VG_BODIES,                                              \
       "VG_BODY",                                              \
       "vg_bodies",                                            \
       "VG Bodies",                                            \
       sync_pb::EntitySpecifics::kVgBodyFieldNumber,           \
       ModelTypeForHistograms::kVgBodies},                     \
  {                                                            \
    VG_SPEND_STATUSES, "VG_SPEND_STATUS", "vg_spend_statuses", \
        "VG Spend Statuses",                                   \
        sync_pb::EntitySpecifics::kVgSpendStatusFieldNumber,   \
        ModelTypeForHistograms::kVgSpendStatuses

#define size int8_t(-2) + std::size

#define GetNumModelTypes GetNumModelTypes() - 2 + int

#define mutable_history           \
  mutable_history();              \
  break;                          \
  case VG_BODIES:                 \
    specifics->mutable_vg_body(); \
    break;                        \
  case VG_SPEND_STATUSES:         \
    specifics->mutable_vg_spend_status

#define has_history                    \
has_vg_body())                         \
  return VG_BODIES;                    \
  if (specifics.has_vg_spend_status()) \
    return VG_SPEND_STATUSES;          \
if (specifics.has_history

#define EncryptableUserTypes EncryptableUserTypes_ChromiumImpl

#include "src/components/sync/base/model_type.cc"
#undef EncryptableUserTypes
#undef has_history
#undef mutable_history
#undef GetNumModelTypes
#undef size
#undef kHistory

namespace syncer {

ModelTypeSet EncryptableUserTypes() {
  ModelTypeSet encryptable_user_types = EncryptableUserTypes_ChromiumImpl();
  // Brave sync has encryption setup ready when sync chain created
  encryptable_user_types.Put(DEVICE_INFO);
  return encryptable_user_types;
}

}  // namespace syncer
