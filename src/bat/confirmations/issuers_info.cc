/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/issuers_info.h"

namespace confirmations {

IssuersInfo::IssuersInfo() :
    public_key(""),
    issuers({}) {}

IssuersInfo::IssuersInfo(const IssuersInfo& info) :
    public_key(info.public_key),
    issuers(info.issuers) {}

IssuersInfo::~IssuersInfo() = default;

}  // namespace confirmations
