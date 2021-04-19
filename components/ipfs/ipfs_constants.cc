/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_constants.h"

namespace ipfs {

const char kSwarmPeersPath[] = "/api/v0/swarm/peers";
const char kConfigPath[] = "/api/v0/config";
const char kArgQueryParam[] = "arg";
const char kAddressesField[] = "Addresses";
const char kShutdownPath[] = "/api/v0/shutdown";
const char kIPFSScheme[] = "ipfs";
const char kIPNSScheme[] = "ipns";
const char kDefaultIPFSGateway[] = "https://dweb.link";
const char kDefaultIPFSLocalGateway[] = "http://localhost";
const char kIPFSSettingsURL[] = "brave://settings/ipfs";
const char kIPFSLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/sections/"
    "360010974932-InterPlanetary-File-System-IPFS-";
const char kRepoStatsPath[] = "/api/v0/repo/stat";
const char kRepoStatsHumanReadableParamName[] = "human";
const char kRepoStatsHumanReadableParamValue[] = "true";
const char kNodeInfoPath[] = "/api/v0/id";
const char kLocalhostIP[] = "127.0.0.1";
const char kLocalhostDomain[] = "localhost";
const char kGarbageCollectionPath[] = "/api/v0/repo/gc";
const char kImportAddPath[] = "/api/v0/add";
const char kImportMakeDirectoryPath[] = "/api/v0/files/mkdir";
const char kImportCopyPath[] = "/api/v0/files/cp";
const char kImportDirectory[] = "/brave-imports/";
const char kIPFSImportMultipartContentType[] = "multipart/form-data;";
const char kFileValueName[] = "file";
const char kFileMimeType[] = "application/octet-stream";

}  // namespace ipfs
