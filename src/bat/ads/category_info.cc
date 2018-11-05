/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/category_info.h"

namespace ads {

CategoryInfo::CategoryInfo() :
    creative_set_id(""),
    advertiser(""),
    notification_text(""),
    notification_url(""),
    uuid("") {}

CategoryInfo::CategoryInfo(const CategoryInfo& info) :
    creative_set_id(info.creative_set_id),
    advertiser(info.advertiser),
    notification_text(info.notification_text),
    notification_url(info.notification_url),
    uuid(info.uuid) {}

CategoryInfo::~CategoryInfo() {}

}  // namespace ads
