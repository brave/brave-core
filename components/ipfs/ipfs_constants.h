/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_CONSTANTS_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_CONSTANTS_H_

namespace ipfs {

extern const char kSwarmPeersPath[];
extern const char kConfigPath[];
extern const char kArgQueryParam[];
extern const char kAddressesField[];
extern const char kShutdownPath[];
extern const char kIPFSScheme[];
extern const char kIPNSScheme[];
extern const char kDefaultIPFSGateway[];
extern const char kDefaultIPFSLocalGateway[];
extern const char kIPFSLearnMoreURL[];
extern const char kIPFSSettingsURL[];
extern const char kRepoStatsPath[];
extern const char kRepoStatsHumanReadableParamName[];
extern const char kRepoStatsHumanReadableParamValue[];
extern const char kNodeInfoPath[];
extern const char kLocalhostIP[];
extern const char kLocalhostDomain[];
extern const char kGarbageCollectionPath[];
extern const char kImportAddPath[];
extern const char kImportMakeDirectoryPath[];
extern const char kImportCopyPath[];
extern const char kImportDirectory[];
extern const char kIPFSImportMultipartContentType[];
extern const char kFileValueName[];
extern const char kFileMimeType[];

// Keep it synced with IPFSResolveMethodTypes in
// browser/resources/settings/brave_ipfs_page/brave_ipfs_page.js
enum class IPFSResolveMethodTypes {
  IPFS_ASK,
  IPFS_GATEWAY,
  IPFS_LOCAL,
  IPFS_DISABLED,
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_CONSTANTS_H_
