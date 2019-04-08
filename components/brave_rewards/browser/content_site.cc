/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/browser/content_site.h"

namespace brave_rewards {

  ContentSite::ContentSite() : percentage(0) {}

  ContentSite::ContentSite(const std::string& site_id) :
      id(site_id),
      percentage(0),
      verified(false),
      excluded(0),
      weight(0),
      reconcile_stamp(0) {
  }

  ContentSite::~ContentSite() {}

  ContentSite::ContentSite(const ContentSite &properties) {
    percentage = properties.percentage;
    verified = properties.verified;
    excluded = properties.excluded;
    name = properties.name;
    favicon_url = properties.favicon_url;
    url = properties.url;
    provider = properties.provider;
    id = properties.id;
    weight = properties.weight;
    reconcile_stamp = properties.reconcile_stamp;
  }

}  // namespace brave_rewards
