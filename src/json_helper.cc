/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "json_helper.h"

namespace helper {

bool JSON::Validate(
    rapidjson::Document *document,
    const std::string& jsonSchema) {
  if (document->HasParseError()) {
    return false;
  }

  rapidjson::Document document_schema;
  document_schema.Parse(jsonSchema.c_str());

  if (document_schema.HasParseError()) {
    return false;
  }

  rapidjson::SchemaDocument schema(document_schema);
  rapidjson::SchemaValidator validator(schema);
  if (!document->Accept(validator)) {
    return false;
  }

  return true;
}

}  // namespace helper
