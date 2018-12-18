/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_SEARCH_PROVIDER_INFO_H_
#define BAT_ADS_SEARCH_PROVIDER_INFO_H_

#include <string>

namespace ads {

struct SearchProviderInfo {
 public:
  SearchProviderInfo() :
      name(""),
      hostname(""),
      search_template(""),
      is_always_classed_as_a_search(false) {}

  SearchProviderInfo(
      const std::string& name,
      const std::string& hostname,
      const std::string& search_template,
      bool is_always_classed_as_a_search) :
      name(name),
      hostname(hostname),
      search_template(search_template),
      is_always_classed_as_a_search(is_always_classed_as_a_search) {}

  SearchProviderInfo(const SearchProviderInfo& info) :
      name(info.name),
      hostname(info.hostname),
      search_template(info.search_template),
      is_always_classed_as_a_search(info.is_always_classed_as_a_search) {}

  ~SearchProviderInfo() {}

  std::string name;
  std::string hostname;
  std::string search_template;
  bool is_always_classed_as_a_search;
};

}  // namespace ads

#endif  // BAT_ADS_SEARCH_PROVIDER_INFO_H_
