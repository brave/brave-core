/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

namespace ads {

struct SearchProviderInfo {
  SearchProviderInfo() :
    name(""),
    base(""),
    search(""),
    any_visit_to_base_domain_is_search(false) {}

  SearchProviderInfo(
      const std::string& name,
      const std::string& base,
      const std::string& search,
      bool any_visit_to_base_domain_is_search) :
    name(name),
    base(base),
    search(search),
    any_visit_to_base_domain_is_search(any_visit_to_base_domain_is_search) {}

  SearchProviderInfo(const SearchProviderInfo& info) :
    name(info.name),
    base(info.base),
    search(info.search),
    any_visit_to_base_domain_is_search(
      info.any_visit_to_base_domain_is_search) {}

  ~SearchProviderInfo() {}

  std::string name;
  std::string base;
  std::string search;
  bool any_visit_to_base_domain_is_search;
};

}  // namespace ads
