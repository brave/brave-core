/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_CONTENT_SITE_
#define BRAVE_BROWSER_PAYMENTS_CONTENT_SITE_

#include <string>

#include "base/macros.h"

namespace ledger {
struct PublisherInfo;
}

namespace payments {

struct ContentSite {
  typedef std::string id_type;
  ContentSite(const id_type site_id);

  const id_type id;
  double score;
  bool pinned;
  double percentage;
  bool excluded;
};

ContentSite PublisherInfoToContentSite(
    const ledger::PublisherInfo& publisher_info);

typedef std::vector<const ContentSite> ContentSiteList;

}  // namespace payments

#endif  // BRAVE_BROWSER_PAYMENTS_CONTENT_SITE_
