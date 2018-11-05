/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT UrlComponents {
  UrlComponents() :
    url(""),
    scheme(""),
    user(""),
    hostname(""),
    port(""),
    query(""),
    fragment(""),
    absolute_path(false) {}

  explicit UrlComponents(const UrlComponents& components) :
    url(components.url),
    scheme(components.scheme),
    user(components.user),
    hostname(components.hostname),
    port(components.port),
    query(components.query),
    fragment(components.fragment),
    absolute_path(components.absolute_path) {}

  ~UrlComponents() {}

  std::string url;
  std::string scheme;
  std::string user;
  std::string hostname;
  std::string port;
  std::string query;
  std::string fragment;
  bool absolute_path;
};

}  // namespace ads
