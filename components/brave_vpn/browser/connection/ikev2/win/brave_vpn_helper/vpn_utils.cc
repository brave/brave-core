// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_vpn/browser/connection/ikev2/win/brave_vpn_helper/vpn_utils.h"

#include <fwpmu.h>
#include <iphlpapi.h>
#include <ras.h>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "base/win/windows_version.h"
#include "brave/components/brave_vpn/browser/connection/common/win/scoped_sc_handle.h"
#include "brave/components/brave_vpn/browser/connection/common/win/utils.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

namespace {

// Microsoft-Windows-NetworkProfile
// fbcfac3f-8459-419f-8e48-1f0b49cdb85e
constexpr GUID kNetworkProfileGUID = {
    0xfbcfac3f,
    0x8459,
    0x419f,
    {0x8e, 0x48, 0x1f, 0x0b, 0x49, 0xcd, 0xb8, 0x5e}};

bool SetServiceTriggerForVPNConnection(SC_HANDLE hService,
                                       const std::wstring& brave_vpn_entry) {
  std::wstring brave_vpn_entry_with_null(brave_vpn_entry);
  brave_vpn_entry_with_null += L'\0';
  // Allocate and set the SERVICE_TRIGGER_SPECIFIC_DATA_ITEM structure
  SERVICE_TRIGGER_SPECIFIC_DATA_ITEM deviceData = {0};
  deviceData.dwDataType = SERVICE_TRIGGER_DATA_TYPE_STRING;
  // Exclude EOL
  deviceData.cbData = brave_vpn_entry_with_null.size() *
                      sizeof(brave_vpn_entry_with_null.front());
  deviceData.pData = (PBYTE)brave_vpn_entry_with_null.c_str();
  // Allocate and set the SERVICE_TRIGGER structure
  SERVICE_TRIGGER serviceTrigger = {0};
  serviceTrigger.dwTriggerType = SERVICE_TRIGGER_TYPE_CUSTOM;
  serviceTrigger.dwAction = SERVICE_TRIGGER_ACTION_SERVICE_START;
  serviceTrigger.pTriggerSubtype = const_cast<GUID*>(&kNetworkProfileGUID);
  serviceTrigger.cDataItems = 1;
  serviceTrigger.pDataItems = &deviceData;

  // Allocate and set the SERVICE_TRIGGER_INFO structure
  SERVICE_TRIGGER_INFO serviceTriggerInfo = {0};
  serviceTriggerInfo.cTriggers = 1;
  serviceTriggerInfo.pTriggers = &serviceTrigger;

  // Call ChangeServiceConfig2 with the SERVICE_CONFIG_TRIGGER_INFO level
  // and pass to it the address of the SERVICE_TRIGGER_INFO structure
  return ChangeServiceConfig2(hService, SERVICE_CONFIG_TRIGGER_INFO,
                              &serviceTriggerInfo);
}

bool SetServiceFailActions(SC_HANDLE service) {
  SC_ACTION failActions[] = {
      {SC_ACTION_RESTART, 1}, {SC_ACTION_RESTART, 1}, {SC_ACTION_RESTART, 1}};
  // The time after which to reset the failure count to zero if there are no
  // failures, in seconds.
  SERVICE_FAILURE_ACTIONS servFailActions = {
      .dwResetPeriod = 0,
      .lpRebootMsg = NULL,
      .lpCommand = NULL,
      .cActions = sizeof(failActions) / sizeof(SC_ACTION),
      .lpsaActions = failActions};
  return ChangeServiceConfig2(service, SERVICE_CONFIG_FAILURE_ACTIONS,
                              &servFailActions);
}

DWORD AddSublayer(GUID uuid) {
  FWPM_SESSION0 session = {};
  HANDLE engine = nullptr;
  auto result =
      FwpmEngineOpen0(nullptr, RPC_C_AUTHN_WINNT, nullptr, &session, &engine);
  if (result == ERROR_SUCCESS) {
    std::wstring name(kBraveVPNServiceFilter);
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

absl::optional<int> GetAdapterIndexByName(const std::string& name) {
  ULONG adapter_info_size = 0;
  // Get the right buffer size in case of overflow
  if (::GetAdaptersInfo(nullptr, &adapter_info_size) != ERROR_BUFFER_OVERFLOW ||
      adapter_info_size == 0) {
    return absl::nullopt;
  }

  std::vector<byte> adapters(adapter_info_size);
  if (::GetAdaptersInfo(reinterpret_cast<PIP_ADAPTER_INFO>(adapters.data()),
                        &adapter_info_size) != ERROR_SUCCESS) {
    return absl::nullopt;
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

  return absl::nullopt;
}

DWORD BlockIPv4Queries(HANDLE engine_handle) {
  std::vector<FWPM_FILTER_CONDITION0> conditions = {
      {FWPM_CONDITION_IP_REMOTE_PORT,
       FWP_MATCH_EQUAL,
       {FWP_UINT16, {.uint16 = 53}}}};

  FWPM_FILTER0 filter = {};
  filter.subLayerKey = kVpnDnsSublayerGUID;
  std::wstring name(kBraveVPNServiceFilter);
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
  Filter.subLayerKey = kVpnDnsSublayerGUID;
  std::wstring name(kBraveVPNServiceFilter);
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

  std::vector<FWPM_FILTER_CONDITION0> conditions = {
      {FWPM_CONDITION_IP_REMOTE_PORT,
       FWP_MATCH_EQUAL,
       {FWP_UINT16, {.uint16 = 53}}},
      {FWPM_CONDITION_IP_LOCAL_INTERFACE,
       FWP_MATCH_EQUAL,
       {FWP_UINT64, {.uint64 = &tapluid.Value}}}};

  FWPM_FILTER0 Filter = {};
  Filter.subLayerKey = kVpnDnsSublayerGUID;
  std::wstring name(kBraveVPNServiceFilter);
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
  auto result = RegisterSublayer(engine_handle, kVpnDnsSublayerGUID);
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

bool ConfigureServiceAutoRestart(const std::wstring& service_name,
                                 const std::wstring& brave_vpn_entry) {
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
  if (!scm.IsValid()) {
    LOG(ERROR) << "::OpenSCManager failed. service_name: " << service_name
               << ", error: " << std::hex << HRESULTFromLastError();
    return false;
  }
  ScopedScHandle service(
      ::OpenService(scm.Get(), service_name.c_str(), SERVICE_ALL_ACCESS));
  if (!service.IsValid()) {
    LOG(ERROR) << "::OpenService failed. service_name: " << service_name
               << ", error: " << std::hex << HRESULTFromLastError();
    return false;
  }

  if (!SetServiceFailActions(service.Get())) {
    LOG(ERROR) << "SetServiceFailActions failed:" << std::hex
               << HRESULTFromLastError();
    return false;
  }
  if (!SetServiceTriggerForVPNConnection(service.Get(), brave_vpn_entry)) {
    LOG(ERROR) << "SetServiceTriggerForVPNConnection failed:" << std::hex
               << HRESULTFromLastError();
    return false;
  }
  return true;
}

void SetFiltersInstalledFlag() {
  base::win::RegKey key(HKEY_LOCAL_MACHINE, kBraveVpnHelperRegistryStoragePath,
                        KEY_ALL_ACCESS);
  if (!key.Valid()) {
    VLOG(1) << "Failed to open vpn service storage";
    return;
  }
  DWORD launch = 1;
  key.WriteValue(kBraveVpnHelperFiltersInstalledValue, launch);
}

void ResetFiltersInstalledFlag() {
  base::win::RegKey key(HKEY_LOCAL_MACHINE, kBraveVpnHelperRegistryStoragePath,
                        KEY_ALL_ACCESS);
  if (!key.Valid()) {
    VLOG(1) << "Failed to open vpn service storage";
    return;
  }
  key.DeleteValue(kBraveVpnHelperFiltersInstalledValue);
}

}  // namespace brave_vpn
