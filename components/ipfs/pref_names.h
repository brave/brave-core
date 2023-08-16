/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_PREF_NAMES_H_
#define BRAVE_COMPONENTS_IPFS_PREF_NAMES_H_

extern const char kIPFSAutoRedirectToConfiguredGateway[];
extern const char kIPFSBinaryPath[];
extern const char kIPFSEnabled[];
extern const char kIPFSInfobarCount[];
extern const char kIPFSLocalNodeUsed[];
extern const char kIPFSPublicGatewayAddress[];
extern const char kIPFSPublicNFTGatewayAddress[];
extern const char kIPFSResolveMethod[];
extern const char kIpfsStorageMax[];
extern const char kIPFSPinnedCids[];
extern const char kIPFSAutoFallbackToGateway[];
extern const char kShowIPFSPromoInfobar[];

// Deprecated, use kIPFSAutoRedirectToConfiguredGateway instead
extern const char kIPFSAutoRedirectDNSLink[];
extern const char kIPFSAutoRedirectGateway[];

#endif  // BRAVE_COMPONENTS_IPFS_PREF_NAMES_H_
