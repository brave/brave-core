/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_VPN_BRAVE_VPN_LOCALIZED_STRING_PROVIDER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_VPN_BRAVE_VPN_LOCALIZED_STRING_PROVIDER_H_

namespace content {
class WebUIDataSource;
}
namespace brave_vpn {

void AddLocalizedStrings(content::WebUIDataSource* html_source);

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_VPN_BRAVE_VPN_LOCALIZED_STRING_PROVIDER_H_
