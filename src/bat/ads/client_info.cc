/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/client_info.h"

namespace ads {

ClientInfo::ClientInfo() :
    application_version(""),
    platform(ClientInfoPlatformType::UNKNOWN),
    platform_version("") {}

ClientInfo::ClientInfo(const ClientInfo& info) :
    application_version(info.application_version),
    platform(info.platform),
    platform_version(info.platform_version) {}

ClientInfo::~ClientInfo() = default;

}  // namespace ads
