/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_URL_REPLACEMENTS_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_URL_REPLACEMENTS_H_

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "url/gurl.h"

namespace endpoint_client {

// A self-contained, owning set of per-request URL overrides.
//
// GURL::Replacements holds string_views into buffers the caller must keep
// alive; it is designed to be built and applied within a single expression,
// never stored. Storing one as a member would make the enclosing object
// non-self-contained and prone to dangling views.
//
// UrlReplacements instead records each Set*() or Clear*() call together with
// the string it owns, so it is self-contained and safe to store, copy, and
// move. It never hands out a GURL::Replacements; Apply() builds one
// transiently by replaying the calls, uses it to rewrite the given base URL,
// and discards it - the string_views never outlive the expression.
//
// As with GURL::Replacements, Set*() replaces a component (an empty value
// leaves it empty, e.g. a trailing "?" for the query) and Clear*() removes it.
// There's no ClearScheme(), since a URL can't have no scheme.
//
// Set*() and Clear*() for the remaining components (username, password, ref)
// can be added when a caller needs them.
class UrlReplacements {
 public:
  UrlReplacements();

  UrlReplacements(const UrlReplacements&);
  UrlReplacements& operator=(const UrlReplacements&);

  UrlReplacements(UrlReplacements&&);
  UrlReplacements& operator=(UrlReplacements&&);

  ~UrlReplacements();

  UrlReplacements& SetScheme(std::string scheme);

  UrlReplacements& SetHost(std::string host);
  UrlReplacements& ClearHost();

  UrlReplacements& SetPort(std::string port);
  UrlReplacements& ClearPort();

  UrlReplacements& SetPath(std::string path);
  UrlReplacements& ClearPath();

  UrlReplacements& SetQuery(std::string query);
  UrlReplacements& ClearQuery();

  // Returns |url| with the replacements applied, in the order they were made.
  // A default-constructed UrlReplacements leaves |url| unchanged.
  GURL Apply(const GURL& url) const;

 private:
  struct Set {
    using Function = void (GURL::Replacements::*)(std::string_view);

    Function function;
    std::string value;
  };

  struct Clear {
    using Function = void (GURL::Replacements::*)();

    Function function;
  };

  using Replacement = std::variant<Set, Clear>;

  UrlReplacements& Add(Replacement replacement);

  std::vector<Replacement> replacements_;
};

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_URL_REPLACEMENTS_H_
