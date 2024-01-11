/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/wireguard/win/service_details.h"

#include <guiddef.h>

#include <vector>

#include "base/containers/cxx20_erase.h"
#include "base/path_service.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"
#include "build/build_config.h"
#include "components/version_info/version_info.h"

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

#if BUILDFLAG(CHANNEL_NIGHTLY)
constexpr wchar_t kBraveWireguardTunnelServiceName[] =
    L"BraveVpnNightlyWireguardTunnelService";
// A8D57D90-7A29-4405-91D7-A712F347E426
constexpr CLSID kBraveWireguardServiceCLSID = {
    0xa8d57d90,
    0x7a29,
    0x4405,
    {0x91, 0xd7, 0xa7, 0x12, 0xf3, 0x47, 0xe4, 0x26}};
#elif BUILDFLAG(CHANNEL_BETA)
constexpr wchar_t kBraveWireguardTunnelServiceName[] =
    L"BraveVpnBetaWireguardTunnelService";
// 93175676-5FAC-4D73-B1E1-5485003C9427
constexpr CLSID kBraveWireguardServiceCLSID = {
    0x93175676,
    0x5fac,
    0x4d73,
    {0xb1, 0xe1, 0x54, 0x85, 0x00, 0x3c, 0x94, 0x27}};
#elif BUILDFLAG(CHANNEL_DEV)
constexpr wchar_t kBraveWireguardTunnelServiceName[] =
    L"BraveVpnDevWireguardTunnelService";
// 52C95DE1-D7D9-4C03-A275-8A4517AFAE08
constexpr CLSID kBraveWireguardServiceCLSID = {
    0x52c95de1,
    0xd7d9,
    0x4c03,
    {0xa2, 0x75, 0x8a, 0x45, 0x17, 0xaf, 0xae, 0x08}};
#elif BUILDFLAG(CHANNEL_DEVELOPMENT)
constexpr wchar_t kBraveWireguardTunnelServiceName[] =
    L"BraveVpnDevelopmentWireguardTunnelService";
// 57B73EDD-CBE4-46CA-8ACB-11D90840AF6E
constexpr CLSID kBraveWireguardServiceCLSID = {
    0x57b73edd,
    0xcbe4,
    0x46ca,
    {0x8a, 0xcb, 0x11, 0xd9, 0x08, 0x40, 0xaf, 0x6e}};
#else
constexpr wchar_t kBraveWireguardTunnelServiceName[] =
    L"BraveVpnWireguardTunnelService";

// 088C5F6E-B213-4A8E-98AD-9D64D8913968
constexpr CLSID kBraveWireguardServiceCLSID = {
    0x088c5f6e,
    0xb213,
    0x4a8e,
    {0x98, 0xad, 0x9d, 0x64, 0xd8, 0x91, 0x39, 0x68}};
#endif

}  // namespace

// Returns the Brave Vpn Service CLSID, IID, Name, and Display Name
// respectively.
const CLSID& GetBraveVpnWireguardServiceClsid() {
  return kBraveWireguardServiceCLSID;
}

const IID& GetBraveVpnWireguardServiceIid() {
  return kBraveWireguardServiceIID;
}

std::wstring GetBraveVpnWireguardServiceDisplayName() {
// TODO(simonhong): Use ChannelInfo instead of BUILDFLAG.
#if BUILDFLAG(CHANNEL_NIGHTLY)
  return L"Brave Nightly Vpn Wireguard Service";
#elif BUILDFLAG(CHANNEL_BETA)
  return L"Brave Beta Vpn Wireguard Service";
#elif BUILDFLAG(CHANNEL_DEV)
  return L"Brave Dev Vpn Wireguard Service";
#elif BUILDFLAG(CHANNEL_DEVELOPMENT)
  return L"Brave Development Vpn Wireguard Service";
#else
  return L"Brave Vpn Wireguard Service";
#endif
}

std::wstring GetBraveVpnWireguardServiceName() {
  std::wstring name = GetBraveVpnWireguardServiceDisplayName();
  std::erase_if(name, isspace);
  return name;
}

std::wstring GetBraveVpnWireguardTunnelServiceName() {
  return kBraveWireguardTunnelServiceName;
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
