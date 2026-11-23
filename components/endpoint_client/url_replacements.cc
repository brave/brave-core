/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/url_replacements.h"

#include <utility>
#include <variant>

#include "third_party/abseil-cpp/absl/functional/overload.h"

namespace endpoint_client {

UrlReplacements::UrlReplacements() = default;

UrlReplacements::UrlReplacements(const UrlReplacements&) = default;
UrlReplacements& UrlReplacements::operator=(const UrlReplacements&) = default;

UrlReplacements::UrlReplacements(UrlReplacements&&) = default;
UrlReplacements& UrlReplacements::operator=(UrlReplacements&&) = default;

UrlReplacements::~UrlReplacements() = default;

UrlReplacements& UrlReplacements::SetScheme(std::string scheme) {
  return Add(Set{&GURL::Replacements::SetSchemeStr, std::move(scheme)});
}

UrlReplacements& UrlReplacements::SetHost(std::string host) {
  return Add(Set{&GURL::Replacements::SetHostStr, std::move(host)});
}

UrlReplacements& UrlReplacements::ClearHost() {
  return Add(Clear{&GURL::Replacements::ClearHost});
}

UrlReplacements& UrlReplacements::SetPort(std::string port) {
  return Add(Set{&GURL::Replacements::SetPortStr, std::move(port)});
}

UrlReplacements& UrlReplacements::ClearPort() {
  return Add(Clear{&GURL::Replacements::ClearPort});
}

UrlReplacements& UrlReplacements::SetPath(std::string path) {
  return Add(Set{&GURL::Replacements::SetPathStr, std::move(path)});
}

UrlReplacements& UrlReplacements::ClearPath() {
  return Add(Clear{&GURL::Replacements::ClearPath});
}

UrlReplacements& UrlReplacements::SetQuery(std::string query) {
  return Add(Set{&GURL::Replacements::SetQueryStr, std::move(query)});
}

UrlReplacements& UrlReplacements::ClearQuery() {
  return Add(Clear{&GURL::Replacements::ClearQuery});
}

GURL UrlReplacements::Apply(const GURL& url) const {
  GURL::Replacements replacements;
  for (const auto& replacement : replacements_) {
    std::visit(
        absl::Overload{
            [&](const Set& set) { (replacements.*set.function)(set.value); },
            [&](const Clear& clear) { (replacements.*clear.function)(); },
        },
        replacement);
  }
  return url.ReplaceComponents(replacements);
}

UrlReplacements& UrlReplacements::Add(Replacement replacement) {
  replacements_.push_back(std::move(replacement));
  return *this;
}

}  // namespace endpoint_client
