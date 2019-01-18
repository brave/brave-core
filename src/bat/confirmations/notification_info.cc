/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/notification_info.h"

namespace confirmations {

NotificationInfo::NotificationInfo() :
    creative_set_id(""),
    category(""),
    advertiser(""),
    text(""),
    url(""),
    uuid("") {}

NotificationInfo::NotificationInfo(const NotificationInfo& info) :
    creative_set_id(info.creative_set_id),
    category(info.category),
    advertiser(info.advertiser),
    text(info.text),
    url(info.url),
    uuid(info.uuid) {}

NotificationInfo::~NotificationInfo() = default;

}  // namespace confirmations
