/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"

#include "base/memory/scoped_refptr.h"
#include "build/build_config.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_vpn {

#if BUILDFLAG(IS_ANDROID)
// To avoid complicated build flag checking, provide empty implementation on
// android.
std::unique_ptr<BraveVPNOSConnectionAPI> CreateBraveVPNOSConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel) {
  return nullptr;
}
#endif

}  // namespace brave_vpn
