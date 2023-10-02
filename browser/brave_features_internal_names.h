/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_FEATURES_INTERNAL_NAMES_H_
#define BRAVE_BROWSER_BRAVE_FEATURES_INTERNAL_NAMES_H_

#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "build/build_config.h"

constexpr char kPlaylistFeatureInternalName[] = "playlist";
constexpr char kPlaylistFakeUAFeatureInternalName[] = "playlist-fake-ua";

#if BUILDFLAG(ENABLE_BRAVE_VPN)
constexpr char kBraveVPNFeatureInternalName[] = "brave-vpn";
#if BUILDFLAG(IS_WIN)
constexpr char kBraveVPNDnsFeatureInternalName[] = "brave-vpn-dns";
#endif
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN_WIREGUARD)
constexpr char kBraveVPNWireguardFeatureInternalName[] = "brave-vpn-wireguard";
#endif

#endif  // BRAVE_BROWSER_BRAVE_FEATURES_INTERNAL_NAMES_H_
