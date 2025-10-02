/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/update_client/privacy_preserving_protocol_serializer.h"

#include <string>
#include <utility>

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
  std::optional<base::Value> root = base::JSONReader::Read(
      upstream_result, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!root.has_value() || !root->is_dict()) {
    return upstream_result;
  }

  base::Value::Dict& root_dict = root->GetDict();
  base::Value::Dict* request_dict = root_dict.FindDict("request");
  if (!request_dict) {
    return upstream_result;
  }

  // We don't want to send the information in the hw dictionary, but the
  // protocol specification requires it to be present. All its fields have
  // default values and are therefore optional. We therefore remain
  // spec-compliant by simply sending an empty hw dictionary.
  if (base::Value::Dict* hw_dict = request_dict->FindDict("hw")) {
    hw_dict->clear();
  }

  if (base::Value::List* apps = request_dict->FindList("apps")) {
    for (auto& app : *apps) {
      if (!app.is_dict()) {
        continue;
      }

      base::Value::Dict& app_dict = app.GetDict();
      app_dict.Remove("lang");

      if (base::Value::List* events = app_dict.FindList("events")) {
        for (auto& event : *events) {
          if (event.is_dict()) {
            event.GetDict().Remove("download_time_ms");
          }
        }
      }
    }
  }

  return base::WriteJson(*root).value_or(std::move(upstream_result));
}

}  // namespace update_client
