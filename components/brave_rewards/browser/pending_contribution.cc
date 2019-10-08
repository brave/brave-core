/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/pending_contribution.h"

namespace brave_rewards {

PendingContributionInfo::PendingContributionInfo() :
  amount(0),
  added_date(0) {
}

PendingContributionInfo::~PendingContributionInfo() { }

PendingContributionInfo::PendingContributionInfo(
    const PendingContributionInfo& data) {
  publisher_key = data.publisher_key;
  status = data.status;
  name = data.name;
  favicon_url = data.favicon_url;
  url = data.url;
  provider = data.provider;
  amount = data.amount;
  added_date = data.added_date;
  viewing_id = data.viewing_id;
  type = data.type;
  expiration_date = data.expiration_date;
}

}  // namespace brave_rewards
