/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/update_client/privacy_preserving_protocol_serializer.h"

#include <string>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "components/update_client/protocol_definition.h"
#include "components/update_client/protocol_serializer_json.h"

namespace update_client {

// This method returns the same result as upstream's ProtocolSerializerJSON,
// but with some fields removed that could be used to fingerprint users.
std::string PrivacyPreservingProtocolSerializer::Serialize(
    const protocol_request::Request& request) const {
  std::string upstream_result = ProtocolSerializerJSON().Serialize(request);
  std::optional<base::Value> root = base::JSONReader::Read(upstream_result);
  if (!root.has_value() || !root->is_dict()) {
    return upstream_result;
  }

  base::Value::Dict& root_dict = root->GetDict();
  base::Value::Dict* request_dict = root_dict.FindDict("request");
  if (!request_dict) {
    return upstream_result;
  }

  request_dict->Remove("hw");

  base::Value::List* apps = request_dict->FindList("apps");
  if (apps) {
    for (auto& app : *apps) {
      if (!app.is_dict()) {
        continue;
      }

      base::Value::Dict& app_dict = app.GetDict();
      app_dict.Remove("lang");

      base::Value::List* events = app_dict.FindList("events");
      if (events) {
        for (auto& event : *events) {
          if (event.is_dict()) {
            event.GetDict().Remove("download_time_ms");
          }
        }
      }
    }
  }

  std::string output;
  base::JSONWriter::Write(*root, &output);
  return output;
}

}  // namespace update_client
