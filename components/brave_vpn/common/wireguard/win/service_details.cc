/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/wireguard/win/service_details.h"

#include <guiddef.h>

#include <vector>

#include "base/containers/cxx20_erase.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"
#include "build/build_config.h"

using version_info::Channel;

namespace brave_vpn {

namespace {

// The service installed to %(VersionDir)s\BraveVpnWireguardService
constexpr wchar_t kBraveVpnWireguardServiceSubFolder[] =
    L"BraveVpnWireguardService";

// 053057AB-CF06-4E6C-BBAD-F8DA6436D933
constexpr IID kBraveWireguardServiceIID = {
    0x053057ab,
    0xcf06,
    0x4e6c,
    {0xbb, 0xad, 0xf8, 0xda, 0x64, 0x36, 0xd9, 0x33}};

constexpr wchar_t kBraveNightlyWireguardTunnelServiceName[] =
    L"BraveVpnNightlyWireguardTunnelService";
// A8D57D90-7A29-4405-91D7-A712F347E426
constexpr CLSID kBraveNightlyWireguardServiceCLSID = {
    0xa8d57d90,
    0x7a29,
    0x4405,
    {0x91, 0xd7, 0xa7, 0x12, 0xf3, 0x47, 0xe4, 0x26}};

constexpr wchar_t kBraveBetaWireguardTunnelServiceName[] =
    L"BraveVpnBetaWireguardTunnelService";
// 93175676-5FAC-4D73-B1E1-5485003C9427
constexpr CLSID kBraveBetaWireguardServiceCLSID = {
    0x93175676,
    0x5fac,
    0x4d73,
    {0xb1, 0xe1, 0x54, 0x85, 0x00, 0x3c, 0x94, 0x27}};

constexpr wchar_t kBraveDevWireguardTunnelServiceName[] =
    L"BraveVpnDevWireguardTunnelService";
// 52C95DE1-D7D9-4C03-A275-8A4517AFAE08
constexpr CLSID kBraveDevWireguardServiceCLSID = {
    0x52c95de1,
    0xd7d9,
    0x4c03,
    {0xa2, 0x75, 0x8a, 0x45, 0x17, 0xaf, 0xae, 0x08}};

constexpr wchar_t kBraveDevelopmentWireguardTunnelServiceName[] =
    L"BraveVpnDevelopmentWireguardTunnelService";
// 57B73EDD-CBE4-46CA-8ACB-11D90840AF6E
constexpr CLSID kBraveDevelopmentWireguardServiceCLSID = {
    0x57b73edd,
    0xcbe4,
    0x46ca,
    {0x8a, 0xcb, 0x11, 0xd9, 0x08, 0x40, 0xaf, 0x6e}};

constexpr wchar_t kBraveWireguardTunnelServiceName[] =
    L"BraveVpnWireguardTunnelService";
// 088C5F6E-B213-4A8E-98AD-9D64D8913968
constexpr CLSID kBraveWireguardServiceCLSID = {
    0x088c5f6e,
    0xb213,
    0x4a8e,
    {0x98, 0xad, 0x9d, 0x64, 0xd8, 0x91, 0x39, 0x68}};

}  // namespace

// Returns the Brave Vpn Service CLSID, IID, Name, and Display Name
// respectively.
const CLSID& GetBraveVpnWireguardServiceClsid(version_info::Channel channel) {
  switch (channel) {
    case Channel::CANARY:
      return kBraveNightlyWireguardServiceCLSID;
    case Channel::DEV:
      return kBraveDevWireguardServiceCLSID;
    case Channel::BETA:
      return kBraveBetaWireguardServiceCLSID;
    case Channel::STABLE:
      return kBraveWireguardServiceCLSID;
    default:
      return kBraveDevelopmentWireguardServiceCLSID;
  }

  NOTREACHED_NORETURN();
}

const IID& GetBraveVpnWireguardServiceIid() {
  return kBraveWireguardServiceIID;
}

std::wstring GetBraveVpnWireguardServiceDisplayName(
    version_info::Channel channel) {
  switch (channel) {
    case Channel::CANARY:
      return L"Brave Nightly Vpn Wireguard Service";
    case Channel::DEV:
      return L"Brave Dev Vpn Wireguard Service";
    case Channel::BETA:
      return L"Brave Beta Vpn Wireguard Service";
    case Channel::STABLE:
      return L"Brave Vpn Wireguard Service";
    default:
      return L"Brave Development Vpn Wireguard Service";
  }

  NOTREACHED_NORETURN();
}

std::wstring GetBraveVpnWireguardServiceName(version_info::Channel channel) {
  std::wstring name = GetBraveVpnWireguardServiceDisplayName(channel);
  std::erase_if(name, isspace);
  return name;
}

std::wstring GetBraveVpnWireguardTunnelServiceName(
    version_info::Channel channel) {
  switch (channel) {
    case Channel::CANARY:
      return kBraveNightlyWireguardTunnelServiceName;
    case Channel::DEV:
      return kBraveDevWireguardTunnelServiceName;
    case Channel::BETA:
      return kBraveBetaWireguardTunnelServiceName;
    case Channel::STABLE:
      return kBraveWireguardTunnelServiceName;
    default:
      return kBraveDevelopmentWireguardTunnelServiceName;
  }

  NOTREACHED_NORETURN();
}

base::FilePath GetBraveVPNWireguardServiceInstallationPath(
    const base::FilePath& target_path,
    const base::Version& version) {
  return target_path.AppendASCII(version.GetString())
      .Append(brave_vpn::kBraveVpnWireguardServiceSubFolder)
      .Append(brave_vpn::kBraveVpnWireguardServiceExecutable);
}

base::FilePath GetBraveVPNWireguardServiceExecutablePath() {
  base::FilePath asset_dir = base::PathService::CheckedGet(base::DIR_ASSETS);
  return asset_dir.Append(brave_vpn::kBraveVpnWireguardServiceSubFolder)
      .Append(brave_vpn::kBraveVpnWireguardServiceExecutable);
}

}  // namespace brave_vpn
