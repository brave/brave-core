/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "json_helper.h"

namespace helper {

ads::Result JSON::Validate(
    rapidjson::Document* document,
    const std::string& json_schema) {
  if (!document) {
    return ads::Result::FAILED;
  }

  if (document->HasParseError()) {
    return ads::Result::FAILED;
  }

  rapidjson::Document document_schema;
  document_schema.Parse(json_schema.c_str());

  if (document_schema.HasParseError()) {
    return ads::Result::FAILED;
  }

  rapidjson::SchemaDocument schema(document_schema);
  rapidjson::SchemaValidator validator(schema);
  if (!document->Accept(validator)) {
    return ads::Result::FAILED;
  }

  return ads::Result::SUCCESS;
}

std::string JSON::GetLastError(rapidjson::Document* document) {
  if (!document) {
    return "Invalid document";
  }

  auto parse_error_code = document->GetParseError();
  std::string description(rapidjson::GetParseError_En(parse_error_code));
  std::string error_offset = std::to_string(document->GetErrorOffset());
  return description + " (" + error_offset + ")";
}

}  // namespace helper
