/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_CONSTANTS_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_CONSTANTS_H_

#include <guiddef.h>

#include "brave/components/brave_vpn/common/buildflags/buildflags.h"

namespace brave_vpn {

inline constexpr char kBraveVpnHelperCrashMe[] = "crash-me";
inline constexpr wchar_t kBraveVPNHelperExecutable[] = L"brave_vpn_helper.exe";
inline constexpr wchar_t kBraveVpnHelperFiltersInstalledValue[] = L"filters";
inline constexpr wchar_t kBraveVpnOneTimeServiceCleanupValue[] = L"ran";

// Repeating interval to check the connection is live.
inline constexpr int kCheckConnectionIntervalInSeconds = 3;

/* UUID of WFP sublayer used by all instances */
#if BUILDFLAG(CHANNEL_NIGHTLY)
inline constexpr wchar_t kBraveVPNServiceFilter[] =
    L"Brave VPN Nightly Service DNS Filter";
inline constexpr wchar_t kBraveVpnHelperRegistryStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn\\HelperServiceNightly";
inline constexpr wchar_t kBraveVpnOneTimeServiceCleanupStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn\\OneTimeServiceCleanupNightly";

// 23e10e29-eb83-4d2c-9d77-f6e9b547f39c
inline constexpr GUID kVpnDnsSublayerGUID = {
    0x23e10e29,
    0xeb83,
    0x4d2c,
    {0x9d, 0x77, 0xf6, 0xe9, 0xb5, 0x47, 0xf3, 0x9c}};
#elif BUILDFLAG(CHANNEL_BETA)
inline constexpr wchar_t kBraveVPNServiceFilter[] =
    L"Brave VPN Beta Service DNS Filter";
inline constexpr wchar_t kBraveVpnHelperRegistryStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn\\HelperServiceBeta";
inline constexpr wchar_t kBraveVpnOneTimeServiceCleanupStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn\\OneTimeServiceCleanupBeta";

// fc5fb7bc-e313-4f5e-8052-fe8b150f7de0
inline constexpr GUID kVpnDnsSublayerGUID = {
    0xfc5fb7bc,
    0xe313,
    0x4f5e,
    {0x80, 0x52, 0xfe, 0x8b, 0x15, 0x0f, 0x7d, 0xe0}};
#elif BUILDFLAG(CHANNEL_DEV)
inline constexpr wchar_t kBraveVPNServiceFilter[] =
    L"Brave VPN Dev Service DNS Filter";
inline constexpr wchar_t kBraveVpnHelperRegistryStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn\\HelperServiceDev";
inline constexpr wchar_t kBraveVpnOneTimeServiceCleanupStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn\\OneTimeServiceCleanupDev";
// c448b198-729d-4a89-879b-1cf0cd2460c0
inline constexpr GUID kVpnDnsSublayerGUID = {
    0xc448b198,
    0x729d,
    0x4a89,
    {0x87, 0x9b, 0x1c, 0xf0, 0xcd, 0x24, 0x60, 0xc0}};
#elif BUILDFLAG(CHANNEL_DEVELOPMENT)
inline constexpr wchar_t kBraveVPNServiceFilter[] =
    L"Brave VPN Development Service DNS Filter";
inline constexpr wchar_t kBraveVpnHelperRegistryStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn\\HelperServiceDevelopment";
inline constexpr wchar_t kBraveVpnOneTimeServiceCleanupStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn\\OneTimeServiceCleanupDevelopment";
// 9c14e1f7-692f-495b-95e8-008113d3c0d6
inline constexpr GUID kVpnDnsSublayerGUID = {
    0x9c14e1f7,
    0x692f,
    0x495b,
    {0x95, 0xe8, 0x00, 0x81, 0x13, 0xd3, 0xc0, 0xd6}};
#else
inline constexpr wchar_t kBraveVPNServiceFilter[] =
    L"Brave VPN Service DNS Filter";
inline constexpr wchar_t kBraveVpnHelperRegistryStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn\\HelperService";
inline constexpr wchar_t kBraveVpnOneTimeServiceCleanupStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn\\OneTimeServiceCleanup";
// 754b7cbd-cad3-474e-8d2c-054413fd4509
inline constexpr GUID kVpnDnsSublayerGUID = {
    0x754b7cbd,
    0xcad3,
    0x474e,
    {0x8d, 0x2c, 0x05, 0x44, 0x13, 0xfd, 0x45, 0x09}};
#endif
}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_CONSTANTS_H_
