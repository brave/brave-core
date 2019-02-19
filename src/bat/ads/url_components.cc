/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/url_components.h"

#include "json_helper.h"

namespace ads {

UrlComponents::UrlComponents() :
    url(""),
    scheme(""),
    user(""),
    hostname(""),
    port(""),
    query(""),
    fragment("") {}

UrlComponents::UrlComponents(const UrlComponents& components) :
    url(components.url),
    scheme(components.scheme),
    user(components.user),
    hostname(components.hostname),
    port(components.port),
    query(components.query),
    fragment(components.fragment) {}

UrlComponents::~UrlComponents() = default;

Result UrlComponents::FromJson(
    const std::string& json,
    std::string* error_description) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    if (error_description) {
      *error_description = helper::JSON::GetLastError(&document);
    }

    return FAILED;
  }

  if (document.HasMember("url")) {
    url = document["url"].GetString();
  }

  if (document.HasMember("scheme")) {
    scheme = document["scheme"].GetString();
  }

  if (document.HasMember("user")) {
    user = document["user"].GetString();
  }

  if (document.HasMember("hostname")) {
    hostname = document["hostname"].GetString();
  }

  if (document.HasMember("port")) {
    port = document["port"].GetString();
  }

  if (document.HasMember("query")) {
    query = document["query"].GetString();
  }

  if (document.HasMember("fragment")) {
    fragment = document["fragment"].GetString();
  }

  return SUCCESS;
}

const std::string UrlComponents::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

void SaveToJson(JsonWriter* writer, const UrlComponents& components) {
  writer->StartObject();

  writer->String("url");
  writer->String(components.url.c_str());

  writer->String("scheme");
  writer->String(components.scheme.c_str());

  writer->String("user");
  writer->String(components.user.c_str());

  writer->String("hostname");
  writer->String(components.hostname.c_str());

  writer->String("port");
  writer->String(components.port.c_str());

  writer->String("query");
  writer->String(components.query.c_str());

  writer->String("fragment");
  writer->String(components.fragment.c_str());

  writer->EndObject();
}

}  // namespace ads
