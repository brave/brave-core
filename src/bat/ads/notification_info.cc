/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/notification_info.h"

namespace ads {

NotificationInfo::AdINotificationInfonfo() :
    category(""),
    advertiser(""),
    text(""),
    url(""),
    uuid("") {}

NotificationInfo::NotificationInfo(const NotificationInfo& info) :
    category(info.category),
    advertiser(info.advertiser),
    text(info.text),
    url(info.url),
    uuid(info.uuid) {}

NotificationInfo::~NotificationInfo() {}

}  // namespace ads
