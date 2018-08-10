/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/browser/content_site.h"

namespace payments {

ContentSite::ContentSite(const id_type site_id) :
    id(site_id),
    score(0),
    pinned(false),
    percentage(0),
    excluded(false) {
}

}  // namespace payments
