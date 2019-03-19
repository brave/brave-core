/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_CONTENT_SITE_
#define BRAVE_BROWSER_PAYMENTS_CONTENT_SITE_

#include <string>
#include <vector>

#include "base/macros.h"

namespace brave_rewards {

struct ContentSite {
  ContentSite();
  ContentSite(const std::string& site_id);
  ContentSite(const ContentSite& properties);
  ~ContentSite();

  // DESC sort
  bool operator<(const ContentSite& other) const {
    return percentage > other.percentage;
  }

  std::string id;
  double percentage;
  bool verified;
  int excluded;
  std::string name;
  std::string favicon_url;
  std::string url;
  std::string provider;
  double weight;
  uint64_t reconcile_stamp;
};

typedef std::vector<ContentSite> ContentSiteList;

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_PAYMENTS_CONTENT_SITE_
