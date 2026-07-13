/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_URL_REPLACEMENTS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_URL_REPLACEMENTS_H_

#include <optional>
#include <string>
#include <utility>

#include "url/gurl.h"

namespace brave_account::endpoint_client {

// A self-contained, owning set of per-request URL overrides.
//
// GURL::Replacements holds string_views into buffers the caller must keep
// alive; it is designed to be built and applied within a single expression,
// never stored. Storing one as a member would make the enclosing object
// non-self-contained and prone to dangling views.
//
// UrlReplacements instead owns its replacement strings (setters take
// std::string by value), so it is self-contained and safe to store and move.
// It never hands out a GURL::Replacements; Apply() builds one transiently,
// uses it to rewrite the given base URL, and discards it - the string_views
// never outlive the expression.
//
// This exposes only the components current callers need (host, path, query);
// it is not a 1:1 mirror of GURL::Replacements. Others (scheme, port, etc.)
// can be added if and when a caller needs them.
class UrlReplacements {
 public:
  UrlReplacements();

  UrlReplacements(const UrlReplacements&) = delete;
  UrlReplacements& operator=(const UrlReplacements&) = delete;

  UrlReplacements(UrlReplacements&&);
  UrlReplacements& operator=(UrlReplacements&&);

  ~UrlReplacements();

  UrlReplacements& SetHost(std::string host) {
    host_ = std::move(host);
    return *this;
  }

  UrlReplacements& SetPath(std::string path) {
    path_ = std::move(path);
    return *this;
  }

  UrlReplacements& SetQuery(std::string query) {
    query_ = std::move(query);
    return *this;
  }

  // Returns |url| with the set components replaced.
  // A default-constructed UrlReplacements leaves |url| unchanged.
  GURL Apply(const GURL& url) const {
    GURL::Replacements replacements;

    if (host_) {
      replacements.SetHostStr(*host_);
    }

    if (path_) {
      replacements.SetPathStr(*path_);
    }

    if (query_) {
      replacements.SetQueryStr(*query_);
    }

    return url.ReplaceComponents(replacements);
  }

 private:
  std::optional<std::string> host_;
  std::optional<std::string> path_;
  std::optional<std::string> query_;
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_URL_REPLACEMENTS_H_
