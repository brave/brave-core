/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_gateway.h"

#include <string>

#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_ports.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

namespace {

GURL AppendLocalPort(const std::string& port) {
  GURL gateway = GURL(ipfs::kDefaultIPFSLocalGateway);
  GURL::Replacements replacements;
  replacements.SetPortStr(port);
  return gateway.ReplaceComponents(replacements);
}

}  // namespace

namespace ipfs {

GURL ipfs_default_gateway_for_test;

void SetIPFSDefaultGatewayForTest(const GURL& url) {
  ipfs_default_gateway_for_test = url;
}

GURL GetDefaultIPFSLocalGateway(version_info::Channel channel) {
  return AppendLocalPort(GetGatewayPort(channel));
}

GURL GetDefaultIPFSGateway(content::BrowserContext* context) {
  if (!ipfs_default_gateway_for_test.is_empty()) {
    return GURL(ipfs_default_gateway_for_test);
  }

  DCHECK(context);
  PrefService* prefs = user_prefs::UserPrefs::Get(context);
  return GURL(prefs->GetString(kIPFSPublicGatewayAddress));
}

GURL GetAPIServer(version_info::Channel channel) {
  return AppendLocalPort(GetAPIPort(channel));
}

}  // namespace ipfs
