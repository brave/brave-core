/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/serialize_body.h"

#include "base/check.h"
#include "base/check_deref.h"
#include "base/json/json_writer.h"
#include "google/protobuf/message_lite.h"

namespace endpoint_client::internal {

std::string SerializeBody(const base::DictValue& body) {
  return CHECK_DEREF(base::WriteJson(body));
}

std::string SerializeBody(const google::protobuf::MessageLite& body) {
  std::string serialized;
  CHECK(body.SerializeToString(&serialized));
  return serialized;
}

}  // namespace endpoint_client::internal
