/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_settings_secure_dns_handler.h"

#include "base/feature_list.h"
#include "brave/components/brave_vpn/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace settings {

BraveSecureDnsHandler::BraveSecureDnsHandler() = default;
BraveSecureDnsHandler::~BraveSecureDnsHandler() = default;

void BraveSecureDnsHandler::OnJavascriptAllowed() {
  SecureDnsHandler::OnJavascriptAllowed();
  pref_registrar_.Init(g_browser_process->local_state());
  if (base::FeatureList::IsEnabled(
          brave_vpn::features::kBraveVPNDnsProtection)) {
    pref_registrar_.Add(
        prefs::kBraveVpnDnsConfig,
        base::BindRepeating(
            &BraveSecureDnsHandler::SendSecureDnsSettingUpdatesToJavascript,
            base::Unretained(this)));
  }
}

void BraveSecureDnsHandler::OnJavascriptDisallowed() {
  SecureDnsHandler::OnJavascriptDisallowed();
  pref_registrar_.RemoveAll();
}

}  // namespace settings
