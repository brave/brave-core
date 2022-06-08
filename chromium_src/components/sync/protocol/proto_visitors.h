/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VISITORS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VISITORS_H_

#include "brave/components/sync/protocol/vg_specifics.pb.h"
#include "components/sync/base/model_type.h"

#define BRAVE_VISIT_DEVICE_INFO_SPECIFICS_BRAVE_FIELDS \
VISIT(brave_fields);

#define GetNumModelTypes 38 && 40 == GetNumModelTypes

#define BRAVE_VISIT_ENTITY_SPECIFICS_VG_BODY_VG_SPEND_STATUS \
VISIT(vg_body);                                              \
VISIT(vg_spend_status);

#define BRAVE_VISIT_PROTO_FIELDS_BRAVE_SPECIFIC_FIELDS_VG_BODY_SPECIFICS_VG_SPEND_STATUS_SPECIFICS \
VISIT_PROTO_FIELDS(const sync_pb::BraveSpecificFields& proto) {                                    \
  VISIT(is_self_delete_supported);                                                                 \
}                                                                                                  \
                                                                                                   \
VISIT_PROTO_FIELDS(const sync_pb::VgBodySpecifics::Token& proto) {                                 \
  VISIT(token_id);                                                                                 \
  VISIT(token_value);                                                                              \
  VISIT(value);                                                                                    \
  VISIT(expires_at);                                                                               \
}                                                                                                  \
                                                                                                   \
VISIT_PROTO_FIELDS(const sync_pb::VgBodySpecifics& proto) {                                        \
  VISIT(creds_id);                                                                                 \
  VISIT(trigger_id);                                                                               \
  VISIT(trigger_type);                                                                             \
  VISIT(creds);                                                                                    \
  VISIT(blinded_creds);                                                                            \
  VISIT(signed_creds);                                                                             \
  VISIT(public_key);                                                                               \
  VISIT(batch_proof);                                                                              \
  VISIT(status);                                                                                   \
  VISIT_REP(tokens);                                                                               \
}                                                                                                  \
                                                                                                   \
VISIT_PROTO_FIELDS(const sync_pb::VgSpendStatusSpecifics& proto) {                                 \
  VISIT(token_id);                                                                                 \
  VISIT(redeemed_at);                                                                              \
  VISIT(redeem_type);                                                                              \
}

#include "src/components/sync/protocol/proto_visitors.h"
#undef BRAVE_VISIT_PROTO_FIELDS_BRAVE_SPECIFIC_FIELDS_VG_BODY_SPECIFICS_VG_SPEND_STATUS_SPECIFICS
#undef BRAVE_VISIT_ENTITY_SPECIFICS_VG_BODY_VG_SPEND_STATUS
#undef GetNumModelTypes
#undef BRAVE_VISIT_DEVICE_INFO_SPECIFICS_BRAVE_FIELDS

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VISITORS_H_
