/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CONTENT_SITE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CONTENT_SITE_H_

#include <string>
#include <vector>

#include "base/macros.h"

namespace brave_rewards {

struct ContentSite {
  ContentSite();
  explicit ContentSite(const std::string& site_id);
  ContentSite(const ContentSite& properties);
  ~ContentSite();

  // DESC sort
  bool operator<(const ContentSite& other) const {
    return percentage > other.percentage;
  }

  std::string id;
  double percentage;
  uint32_t status;
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

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CONTENT_SITE_H_
