/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/components/sync/protocol/proto_value_conversions.cc"
#include "brave/components/sync/protocol/vg_specifics.pb.h"

namespace syncer {
std::unique_ptr<base::DictionaryValue> VgBodySpecificsToValue(
    const sync_pb::VgBodySpecifics& proto) {
  return ToValueVisitor().ToValue(proto);
}

std::unique_ptr<base::DictionaryValue> VgSpendStatusSpecificsToValue(
    const sync_pb::VgSpendStatusSpecifics& proto) {
  return ToValueVisitor().ToValue(proto);
}
}  // namespace syncer
