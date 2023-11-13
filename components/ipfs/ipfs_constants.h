/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_CONSTANTS_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_CONSTANTS_H_

namespace ipfs {

inline constexpr char kAPIKeyGenerateEndpoint[] = "/api/v0/key/gen";
inline constexpr char kAPIKeyRemoveEndpoint[] = "/api/v0/key/rm";
inline constexpr char kAPIKeyListEndpoint[] = "/api/v0/key/list";
inline constexpr char kAPIKeyImportEndpoint[] = "/api/v0/key/import";
inline constexpr char kSwarmPeersPath[] = "/api/v0/swarm/peers";
inline constexpr char kAPIPublishNameEndpoint[] = "/api/v0/name/publish";
inline constexpr char kConfigPath[] = "/api/v0/config";
inline constexpr char kArgQueryParam[] = "arg";
inline constexpr char kAddressesField[] = "Addresses";
inline constexpr char kShutdownPath[] = "/api/v0/shutdown";
inline constexpr char kIPFSScheme[] = "ipfs";
inline constexpr char kIPNSScheme[] = "ipns";
inline constexpr char kDefaultIPFSGateway[] = "https://dweb.link";
inline constexpr char kDefaultIPFSNFTGateway[] = "https://nftstorage.link";
inline constexpr char kDefaultIPFSLocalGateway[] = "http://localhost";
inline constexpr char kIPFSSettingsURL[] = "brave://settings/ipfs";
inline constexpr char16_t kIPFSLearnMorePrivacyURL[] =
    u"https://support.brave.com/hc/en-us/articles/"
    u"360051406452-How-does-IPFS-Impact-my-Privacy-";
inline constexpr char kIPFSLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/sections/"
    "360010974932-InterPlanetary-File-System-IPFS-";
inline constexpr char kRepoStatsPath[] = "/api/v0/repo/stat";
inline constexpr char kRepoStatsHumanReadableParamName[] = "human";
inline constexpr char kRepoStatsHumanReadableParamValue[] = "true";
inline constexpr char kNodeInfoPath[] = "/api/v0/id";
inline constexpr char kLocalhostIP[] = "127.0.0.1";
inline constexpr char kLocalhostDomain[] = "localhost";
inline constexpr char kGarbageCollectionPath[] = "/api/v0/repo/gc";
inline constexpr char kImportAddPath[] = "/api/v0/add";
inline constexpr char kImportMakeDirectoryPath[] = "/api/v0/files/mkdir";
inline constexpr char kImportCopyPath[] = "/api/v0/files/cp";
inline constexpr char kImportDirectory[] = "/brave-imports/";
inline constexpr char kIPFSImportMultipartContentType[] =
    "multipart/form-data;";
inline constexpr char kFileValueName[] = "file";
inline constexpr char kFileMimeType[] = "application/octet-stream";
inline constexpr char kDirectoryMimeType[] = "application/x-directory";
inline constexpr char kIPFSImportTextMimeType[] = "application/octet-stream";

// Local pins
inline constexpr char kAddPinPath[] = "api/v0/pin/add";
inline constexpr char kRemovePinPath[] = "api/v0/pin/rm";
inline constexpr char kGetPinsPath[] = "api/v0/pin/ls";

// Keep it synced with IPFSResolveMethodTypes in
// browser/resources/settings/brave_ipfs_page/brave_ipfs_page.js
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.privacy.settings
enum class IPFSResolveMethodTypes {
  IPFS_ASK,
  IPFS_GATEWAY,
  IPFS_LOCAL,
  IPFS_DISABLED,
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_CONSTANTS_H_
