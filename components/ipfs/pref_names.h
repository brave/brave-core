/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_PREF_NAMES_H_
#define BRAVE_COMPONENTS_IPFS_PREF_NAMES_H_

// Used to determine which method should be used to resolve ipfs:// and ipns:///
// schemes, between:
// Ask: Uses a gateway but also prompts the user with an infobar.
// Gateway: Uses a gateway without prompting the user.
// Local: Uses a local node.
// Disabled: Disables all IPFS handling.
inline constexpr char kIPFSResolveMethod[] = "brave.ipfs.resolve_method";
// Stores the location of the IPFS binary
inline constexpr char kIPFSBinaryPath[] = "brave.ipfs.binary_path";

// Used to determine whether to automatically fallback to gateway when the
// local node is not available.
inline constexpr char kIPFSAutoFallbackToGateway[] =
    "brave.ipfs.auto_fallback_to_gateway";

// Deprecated, use kIPFSAutoRedirectToConfiguredGateway instead
// Used to automatically redirect Gateway resources with x-ipfs-path
// header to the configured Brave IPFS gateway.
inline constexpr char kIPFSAutoRedirectGateway[] =
    "brave.ipfs.auto_redirect_gateway";

// The number of times the infobar is shown to ask the user to install IPFS
inline constexpr char kIPFSInfobarCount[] = "brave.ipfs.infobar_count";

// The number of storage used by IPFS Node
inline constexpr char kIpfsStorageMax[] = "brave.ipfs.storage_max";

// Used to enable/disable IPFS via admin policy.
inline constexpr char kIPFSEnabled[] = "brave.ipfs.enabled";

// Used to determine if local node was ever used.
inline constexpr char kIPFSLocalNodeUsed[] = "brave.ipfs.local_node_used";

// Stores IPFS public gateway address to be used when translating IPFS URLs.
inline constexpr char kIPFSPublicGatewayAddress[] =
    "brave.ipfs.public_gateway_address";

// Stores IPFS public gateway address to be used when translating IPFS NFT URLs.
inline constexpr char kIPFSPublicNFTGatewayAddress[] =
    "brave.ipfs.public_nft_gateway_address";

// Stores list of CIDs that are pinned localy
inline constexpr char kIPFSPinnedCids[] = "brave.ipfs.local_pinned_cids";

// Stores info whether IPFS promo infobar was shown yet
inline constexpr char kShowIPFSPromoInfobar[] =
    "brave.ipfs.show_ipfs_promo_infobar";

// Deprecated, use kIPFSAutoRedirectToConfiguredGateway instead
// Used to automatically redirect for DNSLink resources
inline constexpr char kIPFSAutoRedirectDNSLink[] =
    "brave.ipfs.auto_redirect_dnslink";

// This is a new setting which merges kIPFSAutoRedirectGateway and
// kIPFSAutoRedirectDNSLink
inline constexpr char kIPFSAutoRedirectToConfiguredGateway[] =
    "brave.ipfs.auto_redirect_to_configured_gateway";

// Used to determine whether to start IPFS daemon
// at the same moment when Brave starts.
inline constexpr char kIPFSAlwaysStartMode[] = "brave.ipfs.always_start_mode";

// Used to determine if  IPFS always start infobar was ever shown
inline constexpr char kIPFSAlwaysStartInfobarShown[] =
    "brave.ipfs.ipfs_always_start_infobar_shown";

#endif  // BRAVE_COMPONENTS_IPFS_PREF_NAMES_H_
