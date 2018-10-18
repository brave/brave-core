/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/ad_info.h"

namespace confirmations {

AdInfo::AdInfo() :
    creative_set_id(""),
    start_timestamp(""),
    end_timestamp(""),
    regions({}),
    advertiser(""),
    notification_text(""),
    notification_url(""),
    uuid("") {}

AdInfo::AdInfo(const AdInfo& info) :
    creative_set_id(info.creative_set_id),
    start_timestamp(info.start_timestamp),
    end_timestamp(info.end_timestamp),
    regions(info.regions),
    advertiser(info.advertiser),
    notification_text(info.notification_text),
    notification_url(info.notification_url),
    uuid(info.uuid) {}

AdInfo::~AdInfo() = default;

}  // namespace confirmations
