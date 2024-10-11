// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_vpn/win/brave_vpn_helper/vpn_utils.h"

#include <windows.h>

#include <fwpmu.h>
#include <iphlpapi.h>
#include <ras.h>
#include <winerror.h>

#include <array>
#include <ios>
#include <optional>
#include <vector>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/win/registry.h"
#include "base/win/windows_types.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_utils.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/channel.h"

namespace brave_vpn {

namespace {

std::wstring GetBraveVpnServiceFilterName() {
  switch (chrome::GetChannel()) {
    case version_info::Channel::CANARY:
      return L"Brave VPN Nightly Service DNS Filter";
    case version_info::Channel::DEV:
      return L"Brave VPN Dev Service DNS Filter";
    case version_info::Channel::BETA:
      return L"Brave VPN Beta Service DNS Filter";
    case version_info::Channel::STABLE:
      return L"Brave VPN Service DNS Filter";
    case version_info::Channel::UNKNOWN:
      return L"Brave VPN Development Service DNS Filter";
  }

  NOTREACHED_NORETURN();
}

GUID GetVpnDnsSublayerGUID() {
  switch (chrome::GetChannel()) {
    case version_info::Channel::CANARY:
      // 23e10e29-eb83-4d2c-9d77-f6e9b547f39c
      return {0x23e10e29,
              0xeb83,
              0x4d2c,
              {0x9d, 0x77, 0xf6, 0xe9, 0xb5, 0x47, 0xf3, 0x9c}};
    case version_info::Channel::DEV:
      // c448b198-729d-4a89-879b-1cf0cd2460c0
      return {0xc448b198,
              0x729d,
              0x4a89,
              {0x87, 0x9b, 0x1c, 0xf0, 0xcd, 0x24, 0x60, 0xc0}};
    case version_info::Channel::BETA:
      // fc5fb7bc-e313-4f5e-8052-fe8b150f7de0
      return {0xfc5fb7bc,
              0xe313,
              0x4f5e,
              {0x80, 0x52, 0xfe, 0x8b, 0x15, 0x0f, 0x7d, 0xe0}};
    case version_info::Channel::STABLE:
      // 754b7cbd-cad3-474e-8d2c-054413fd4509
      return {0x754b7cbd,
              0xcad3,
              0x474e,
              {0x8d, 0x2c, 0x05, 0x44, 0x13, 0xfd, 0x45, 0x09}};
    case version_info::Channel::UNKNOWN:
      // 9c14e1f7-692f-495b-95e8-008113d3c0d6
      return {0x9c14e1f7,
              0x692f,
              0x495b,
              {0x95, 0xe8, 0x00, 0x81, 0x13, 0xd3, 0xc0, 0xd6}};
  }

  NOTREACHED_NORETURN();
}

DWORD AddSublayer(GUID uuid) {
  FWPM_SESSION0 session = {};
  HANDLE engine = nullptr;
  auto result =
      FwpmEngineOpen0(nullptr, RPC_C_AUTHN_WINNT, nullptr, &session, &engine);
  if (result == ERROR_SUCCESS) {
    std::wstring name(GetBraveVpnServiceFilterName());
    FWPM_SUBLAYER0 sublayer = {};
    sublayer.subLayerKey = uuid;
    sublayer.displayData.name = name.data();
    sublayer.displayData.description = name.data();
    sublayer.flags = 0;
    sublayer.weight = 0x100;

    /* Add sublayer to the session */
    result = FwpmSubLayerAdd0(engine, &sublayer, nullptr);
  }
  if (engine) {
    FwpmEngineClose0(engine);
  }
  return result;
}

DWORD RegisterSublayer(HANDLE engine_handle, GUID uuid) {
  FWPM_SUBLAYER0* sublayer_ptr = nullptr;
  /* Check sublayer exists and add one if it does not. */
  if (FwpmSubLayerGetByKey0(engine_handle, &uuid, &sublayer_ptr) ==
      ERROR_SUCCESS) {
    VLOG(1) << "Using existing sublayer";
    if (sublayer_ptr) {
      FwpmFreeMemory0(reinterpret_cast<void**>(&sublayer_ptr));
    }
    return ERROR_SUCCESS;
  }
  // Add a new sublayer and do not treat "already exists" as an error
  auto result = AddSublayer(uuid);
  if (result != (DWORD)FWP_E_ALREADY_EXISTS && result != ERROR_SUCCESS) {
    VLOG(1) << "Failed to add a persistent sublayer with "
               "BRAVEVPN_DNS_SUBLAYER UUID";
    return result;
  }
  VLOG(1) << "Added a persistent sublayer with BRAVEVPN_DNS_SUBLAYER UUID";
  return ERROR_SUCCESS;
}

std::optional<int> GetAdapterIndexByName(const std::string& name) {
  ULONG adapter_info_size = 0;
  // Get the right buffer size in case of overflow
  if (::GetAdaptersInfo(nullptr, &adapter_info_size) != ERROR_BUFFER_OVERFLOW ||
      adapter_info_size == 0) {
    return std::nullopt;
  }

  std::vector<byte> adapters(adapter_info_size);
  if (::GetAdaptersInfo(reinterpret_cast<PIP_ADAPTER_INFO>(adapters.data()),
                        &adapter_info_size) != ERROR_SUCCESS) {
    return std::nullopt;
  }

  // The returned value is not an array of IP_ADAPTER_INFO elements but a linked
  // list of such
  PIP_ADAPTER_INFO adapter =
      reinterpret_cast<PIP_ADAPTER_INFO>(adapters.data());
  while (adapter) {
    if (std::string(adapter->Description) == name) {
      return adapter->ComboIndex;
    }
    adapter = adapter->Next;
  }

  return std::nullopt;
}

DWORD BlockIPv4Queries(HANDLE engine_handle) {
  std::vector<FWPM_FILTER_CONDITION0> conditions = {
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_REMOTE_PORT,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT16, {.uint16 = 53}}}};

  FWPM_FILTER0 filter = {};
  filter.subLayerKey = GetVpnDnsSublayerGUID();
  std::wstring name = GetBraveVpnServiceFilterName();
  filter.displayData.name = name.data();
  filter.weight.type = FWP_UINT8;
  filter.weight.uint8 = 0xF;
  filter.filterCondition = conditions.data();
  filter.numFilterConditions = conditions.size();

  /* Block all IPv4 DNS queries. */
  filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
  filter.action.type = FWP_ACTION_BLOCK;
  filter.weight.type = FWP_EMPTY;
  filter.numFilterConditions = 1;
  UINT64 filterid = 0;
  return FwpmFilterAdd0(engine_handle, &filter, nullptr, &filterid);
}

// Block all IPv6 DNS queries
DWORD BlockIPv6Queries(HANDLE engine_handle) {
  FWPM_FILTER0 Filter = {};
  Filter.subLayerKey = GetVpnDnsSublayerGUID();
  std::wstring name = GetBraveVpnServiceFilterName();
  Filter.displayData.name = name.data();
  Filter.weight.type = FWP_EMPTY;
  Filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
  Filter.action.type = FWP_ACTION_BLOCK;
  UINT64 filterid = 0;
  return FwpmFilterAdd0(engine_handle, &Filter, nullptr, &filterid);
}

// Permit IPv4 DNS queries from TAP.
// Use a non-zero weight so that the permit filters get higher priority
// over the block filter added with automatic weighting */
DWORD PermitQueriesFromTAP(HANDLE engine_handle,
                           const std::string& connection_name) {
  auto index = GetAdapterIndexByName(connection_name);
  if (!index) {
    VLOG(1) << "Failed to get index for adapter:" << connection_name;
    return ERROR_INVALID_PARAMETER;
  }

  NET_LUID tapluid = {};
  auto result = ConvertInterfaceIndexToLuid(index.value(), &tapluid);
  if (result) {
    VLOG(1) << "Convert interface index to luid failed:" << std::hex << result;
    return result;
  }

  std::array<FWPM_FILTER_CONDITION0, 2u> conditions = {
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_REMOTE_PORT,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT16, {.uint16 = 53}}},
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_LOCAL_INTERFACE,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT64, {.uint64 = &tapluid.Value}}}};

  FWPM_FILTER0 Filter = {};
  Filter.subLayerKey = GetVpnDnsSublayerGUID();
  std::wstring name = GetBraveVpnServiceFilterName();
  Filter.displayData.name = name.data();
  Filter.weight.type = FWP_UINT8;
  Filter.weight.uint8 = 0xE;
  Filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
  Filter.action.type = FWP_ACTION_PERMIT;
  Filter.filterCondition = conditions.data();
  Filter.numFilterConditions = conditions.size();

  UINT64 filterid = 0;
  result = FwpmFilterAdd0(engine_handle, &Filter, nullptr, &filterid);
  if (result) {
    VLOG(1) << "Add filter to permit IPv4 DNS traffic through TAP failed:"
            << std::hex << result;
    return result;
  }

  // Permit IPv6 DNS queries from TAP. Use same weight as IPv4 filter.
  Filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;

  result = FwpmFilterAdd0(engine_handle, &Filter, nullptr, &filterid);
  if (result) {
    VLOG(1) << "Add filter to permit IPv6 DNS traffic through TAP failed:"
            << std::hex << result;
  }
  return result;
}

}  // namespace

bool AddWpmFilters(HANDLE engine_handle, const std::string& connection_name) {
  if (!engine_handle) {
    VLOG(1) << "Engine handle cannot be null";
    return false;
  }
  auto result = RegisterSublayer(engine_handle, GetVpnDnsSublayerGUID());
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "Open FWP session failed, error code:" << std::hex << result;
    return false;
  }

  // Block all IPv4 DNS queries.
  result = BlockIPv4Queries(engine_handle);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "Add filter to block IPv4 DNS traffic failed:" << std::hex
            << result;
    return false;
  }

  // Block all IPv6 DNS queries.
  result = BlockIPv6Queries(engine_handle);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "Add filter to block IPv6 DNS traffic failed:" << std::hex
            << result;
    return false;
  }

  // Permit IPv4 DNS queries from TAP.
  result = PermitQueriesFromTAP(engine_handle, connection_name);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "Add filter to permit IPv4 and IPv6 DNS queries from TAP failed:"
            << std::hex << result;
    return false;
  }

  VLOG(1) << "Added block filters for all interfaces";

  return true;
}

HANDLE OpenWpmSession() {
  FWPM_SESSION0 session = {.flags = FWPM_SESSION_FLAG_DYNAMIC};
  HANDLE engine = nullptr;
  auto result =
      FwpmEngineOpen0(nullptr, RPC_C_AUTHN_WINNT, nullptr, &session, &engine);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "Open FWP session failed, error code:" << std::hex << result;
  }
  return engine;
}

bool CloseWpmSession(HANDLE engine) {
  auto result = FwpmEngineClose0(engine);
  bool success = result == ERROR_SUCCESS;
  if (!success) {
    VLOG(1) << "Failed to close WPM engine, error code:" << std::hex << result;
  }
  return success;
}

bool SubscribeRasConnectionNotification(HANDLE event_handle) {
  // As we pass INVALID_HANDLE_VALUE, we can get connected or disconnected
  // event from any os vpn entry. It's filtered by
  // VpnDnsHandler::OnObjectSignaled().
  auto result = RasConnectionNotificationW(
      static_cast<HRASCONN>(INVALID_HANDLE_VALUE), event_handle,
      RASCN_Connection | RASCN_Disconnection);
  bool success = result == ERROR_SUCCESS;
  if (!success) {
    VLOG(1)
        << "Failed to subscribe for RAS connection notifications, error code:"
        << std::hex << result;
  }
  return success;
}

void SetFiltersInstalledFlag() {
  base::win::RegKey key(HKEY_LOCAL_MACHINE,
                        GetBraveVpnHelperRegistryStoragePath().c_str(),
                        KEY_ALL_ACCESS);
  if (!key.Valid()) {
    VLOG(1) << "Failed to open vpn service storage";
    return;
  }
  DWORD launch = 1;
  key.WriteValue(kBraveVpnHelperFiltersInstalledValue, launch);
}

void ResetFiltersInstalledFlag() {
  base::win::RegKey key(HKEY_LOCAL_MACHINE,
                        GetBraveVpnHelperRegistryStoragePath().c_str(),
                        KEY_ALL_ACCESS);
  if (!key.Valid()) {
    VLOG(1) << "Failed to open vpn service storage";
    return;
  }
  key.DeleteValue(kBraveVpnHelperFiltersInstalledValue);
}

}  // namespace brave_vpn
