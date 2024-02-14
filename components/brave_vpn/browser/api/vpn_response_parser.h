/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_API_VPN_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_API_VPN_RESPONSE_PARSER_H_

#include <string>

#include "base/values.h"

namespace brave_vpn {

std::string ParseSubscriberCredentialFromJson(base::Value records_v,
                                              std::string* error);

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_API_VPN_RESPONSE_PARSER_H_
