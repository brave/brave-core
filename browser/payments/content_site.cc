/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/browser/payments/content_site.h"

#include "bat/ledger/publisher_info.h"

namespace payments {

ContentSite PublisherInfoToContentSite(
    const ledger::PublisherInfo& publisher_info) {
  ContentSite content_site(publisher_info.id);
  content_site.score = publisher_info.score;
  content_site.pinned = publisher_info.pinned;
  content_site.percentage = publisher_info.percent;
  content_site.excluded = publisher_info.excluded;
  return content_site;
}

ContentSite::ContentSite(const id_type site_id) :
    id(site_id),
    score(0),
    pinned(false),
    percentage(0),
    excluded(false) {
}

}  // namespace payments
