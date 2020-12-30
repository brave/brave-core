/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VISITORS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VISITORS_H_

#define BRAVE_VISIT_DEVICE_INFO_SPECIFICS_BRAVE_FIELDS \
VISIT(brave_fields);

#define BRAVE_VISIT_PROTO_FIELDS_BRAVE_SPECIFIC_FIELD                  \
VISIT_PROTO_FIELDS(const sync_pb::BraveSpecificFields& proto) {        \
  VISIT(is_self_delete_supported);                                     \
}

#include "../../../../../components/sync/protocol/proto_visitors.h"

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VISITORS_H_
