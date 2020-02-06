/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/notification_info.h"

namespace confirmations {

NotificationInfo::NotificationInfo() :
    id(""),
    parent_id(""),
    creative_set_id(""),
    category(""),
    advertiser(""),
    text(""),
    url(""),
    uuid(""),
    type(ConfirmationType::UNKNOWN) {}

NotificationInfo::NotificationInfo(const NotificationInfo& info) :
    id(info.id),
    parent_id(info.parent_id),
    creative_set_id(info.creative_set_id),
    category(info.category),
    advertiser(info.advertiser),
    text(info.text),
    url(info.url),
    uuid(info.uuid),
    type(info.type) {}

NotificationInfo::~NotificationInfo() = default;

}  // namespace confirmations
