/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/service_details.h"

#include <guiddef.h>

#include <vector>

#include "base/notreached.h"
#include "base/path_service.h"
#include "brave/browser/brave_vpn/win/service_constants.h"
#include "build/build_config.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/channel.h"

using version_info::Channel;

namespace brave_vpn {

namespace {

// The service installed to %(VersionDir)s\BraveVpnWireguardService
constexpr wchar_t kBraveVpnWireguardServiceSubFolder[] =
    L"BraveVpnWireguardService";

// 6D319801-690B-441E-8C94-5C18D8E7E9D7
constexpr IID kBraveWireguardServiceIID = {
    0x6d319801,
    0x690b,
    0x441e,
    {0x8c, 0x94, 0x5c, 0x18, 0xd8, 0xe7, 0xe9, 0xd7}};

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
const CLSID& GetBraveVpnWireguardServiceClsid() {
  switch (chrome::GetChannel()) {
    case Channel::CANARY:
      return kBraveNightlyWireguardServiceCLSID;
    case Channel::DEV:
      return kBraveDevWireguardServiceCLSID;
    case Channel::BETA:
      return kBraveBetaWireguardServiceCLSID;
    case Channel::STABLE:
      return kBraveWireguardServiceCLSID;
    case Channel::UNKNOWN:
      return kBraveDevelopmentWireguardServiceCLSID;
  }

  NOTREACHED();
}

const IID& GetBraveVpnWireguardServiceIid() {
  return kBraveWireguardServiceIID;
}

std::wstring GetBraveVpnWireguardServiceDisplayName() {
  switch (chrome::GetChannel()) {
    case Channel::CANARY:
      return L"Brave Nightly Vpn Wireguard Service";
    case Channel::DEV:
      return L"Brave Dev Vpn Wireguard Service";
    case Channel::BETA:
      return L"Brave Beta Vpn Wireguard Service";
    case Channel::STABLE:
      return L"Brave Vpn Wireguard Service";
    case Channel::UNKNOWN:
      return L"Brave Development Vpn Wireguard Service";
  }

  NOTREACHED();
}

std::wstring GetBraveVpnWireguardServiceDescription() {
  return L"WireGuard client code for Brave VPN";
}

std::wstring GetBraveVpnWireguardServiceName() {
  std::wstring name = GetBraveVpnWireguardServiceDisplayName();
  std::erase_if(name, isspace);
  return name;
}

std::wstring GetBraveVpnWireguardTunnelServiceName() {
  switch (chrome::GetChannel()) {
    case Channel::CANARY:
      return kBraveNightlyWireguardTunnelServiceName;
    case Channel::DEV:
      return kBraveDevWireguardTunnelServiceName;
    case Channel::BETA:
      return kBraveBetaWireguardTunnelServiceName;
    case Channel::STABLE:
      return kBraveWireguardTunnelServiceName;
    case Channel::UNKNOWN:
      return kBraveDevelopmentWireguardTunnelServiceName;
  }

  NOTREACHED();
}

base::FilePath GetBraveVPNWireguardServiceExecutablePath(
    const base::FilePath& root_dir) {
  return root_dir.Append(brave_vpn::kBraveVpnWireguardServiceSubFolder)
      .Append(brave_vpn::kBraveVpnWireguardServiceExecutable);
}

}  // namespace brave_vpn
