/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "catalog_payload_info.h"

namespace ads {

PayloadInfo::PayloadInfo() :
    body(""),
    title(""),
    target_url("") {}

PayloadInfo::PayloadInfo(const PayloadInfo& info) :
    body(info.body),
    title(info.title),
    target_url(info.target_url) {}

PayloadInfo::~PayloadInfo() {}

}  // namespace ads
