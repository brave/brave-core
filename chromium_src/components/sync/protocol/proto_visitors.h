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

#define GetNumModelTypes 39 && 41 == GetNumModelTypes

#define BRAVE_VISIT_ENTITY_SPECIFICS_VG_BODY_VG_SPEND_STATUS \
  VISIT(vg_body);                                            \
  VISIT(vg_spend_status);

#include "src/components/sync/protocol/proto_visitors.h"
#undef BRAVE_VISIT_ENTITY_SPECIFICS_VG_BODY_VG_SPEND_STATUS
#undef GetNumModelTypes
#undef BRAVE_VISIT_DEVICE_INFO_SPECIFICS_BRAVE_FIELDS

namespace syncer {
template <class V>
// NOLINTNEXTLINE
void VisitProtoFields(V& visitor, const sync_pb::BraveSpecificFields& proto) {
  if (proto.has_is_self_delete_supported())
    visitor.Visit(proto, "is_self_delete_supported",
                  proto.is_self_delete_supported());
}

template <class V>
// NOLINTNEXTLINE
void VisitProtoFields(V& visitor,
                      const sync_pb::VgBodySpecifics::Token& proto) {
  if (proto.has_token_id())
    visitor.Visit(proto, "token_id", proto.token_id());
  if (proto.has_token_value())
    visitor.Visit(proto, "token_value", proto.token_value());
  if (proto.has_value())
    visitor.Visit(proto, "value", proto.value());
  if (proto.has_expires_at())
    visitor.Visit(proto, "expires_at", proto.expires_at());
}

template <class V>
// NOLINTNEXTLINE
void VisitProtoFields(V& visitor, const sync_pb::VgBodySpecifics& proto) {
  if (proto.has_creds_id())
    visitor.Visit(proto, "creds_id", proto.creds_id());
  if (proto.has_trigger_id())
    visitor.Visit(proto, "trigger_id", proto.trigger_id());
  if (proto.has_trigger_type())
    visitor.Visit(proto, "trigger_type", proto.trigger_type());
  if (proto.has_creds())
    visitor.Visit(proto, "creds", proto.creds());
  if (proto.has_blinded_creds())
    visitor.Visit(proto, "blinded_creds", proto.blinded_creds());
  if (proto.has_signed_creds())
    visitor.Visit(proto, "signed_creds", proto.signed_creds());
  if (proto.has_public_key())
    visitor.Visit(proto, "public_key", proto.public_key());
  if (proto.has_batch_proof())
    visitor.Visit(proto, "batch_proof", proto.batch_proof());
  if (proto.has_status())
    visitor.Visit(proto, "status", proto.status());
  visitor.Visit(proto, "tokens", proto.tokens());
}

template <class V>
// NOLINTNEXTLINE
void VisitProtoFields(V& visitor,
                      const sync_pb::VgSpendStatusSpecifics& proto) {
  if (proto.has_token_id())
    visitor.Visit(proto, "token_id", proto.token_id());
  if (proto.has_redeemed_at())
    visitor.Visit(proto, "redeemed_at", proto.redeemed_at());
  if (proto.has_redeem_type())
    visitor.Visit(proto, "redeem_type", proto.redeem_type());
}
}  // namespace syncer

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VISITORS_H_
