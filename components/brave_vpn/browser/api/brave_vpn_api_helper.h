/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_API_BRAVE_VPN_API_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_API_BRAVE_VPN_API_HELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"

namespace base {
class Value;
}  // namespace base

namespace brave_vpn {

struct Hostname;

std::unique_ptr<Hostname> PickBestHostname(
    const std::vector<Hostname>& hostnames);
std::vector<Hostname> ParseHostnames(const base::Value::List& hostnames);
std::string GetTimeZoneName();
base::Value::Dict GetValueWithTicketInfos(
    const std::string& email,
    const std::string& subject,
    const std::string& body,
    const std::string& subscriber_credential,
    const std::string& timezone);

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_API_BRAVE_VPN_API_HELPER_H_
