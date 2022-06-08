/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VALUE_CONVERSIONS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VALUE_CONVERSIONS_H_

#define user_event_specifics user_event_specifics); }                   \
namespace sync_pb {                                                     \
  class VgBodySpecifics;                                                \
  class VgSpendStatusSpecifics;                                         \
}                                                                       \
                                                                        \
namespace syncer {                                                      \
  std::unique_ptr<base::DictionaryValue> VgBodySpecificsToValue(        \
      const sync_pb::VgBodySpecifics& vg_body_specifics);               \
                                                                        \
  std::unique_ptr<base::DictionaryValue> VgSpendStatusSpecificsToValue( \
      const sync_pb::VgSpendStatusSpecifics& vg_spend_status_specifics
#include "src/components/sync/protocol/proto_value_conversions.h"
#undef user_event_specifics

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VALUE_CONVERSIONS_H_
