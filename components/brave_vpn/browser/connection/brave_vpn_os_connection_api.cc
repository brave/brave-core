/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"

#include "base/feature_list.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_vpn/common/features.h"
#include "build/build_config.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_vpn {

std::unique_ptr<BraveVPNOSConnectionAPI> CreateBraveVPNConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel) {
#if BUILDFLAG(IS_WIN)
  if (base::FeatureList::IsEnabled(features::kBraveVPNUseWireguardService)) {
    return CreateBraveVPNWireguardConnectionAPI(url_loader_factory, local_prefs,
                                                channel);
  } else {
    return CreateBraveVPNIKEv2ConnectionAPI(url_loader_factory, local_prefs,
                                            channel);
  }
#elif BUILDFLAG(IS_MAC)
  return CreateBraveVPNIKEv2ConnectionAPI(url_loader_factory, local_prefs,
                                          channel);
#else
  // To avoid complicated build flag checking, provide empty implementation on
  // android.
  return nullptr;
#endif
}

}  // namespace brave_vpn
