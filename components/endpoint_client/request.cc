/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/request.h"

#include "base/check.h"
#include "base/check_deref.h"
#include "base/json/json_writer.h"
#include "google/protobuf/message_lite.h"

namespace endpoint_client::internal {

std::string SerializeJSONRequestBody(const base::DictValue& request_body) {
  return CHECK_DEREF(base::WriteJson(request_body));
}

std::string SerializeProtobufRequestBody(
    const google::protobuf::MessageLite& request_body) {
  std::string serialized;
  CHECK(request_body.SerializeToString(&serialized));
  return serialized;
}

}  // namespace endpoint_client::internal
