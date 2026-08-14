/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_firewall.h"

#include <fwpmu.h>

#include <array>
#include <ios>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"
#include "brave/browser/brave_vpn/win/service_details.h"
#include "brave/components/brave_vpn/common/win/scoped_sc_handle.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/channel.h"

namespace brave_vpn {

namespace wireguard {

namespace {

// Filter weights within our sublayer. The highest matching weight decides, and
// a block beats a permit at equal weight, so anything whose ordering matters
// gets a distinct value.
//
// The only ordering that is load bearing: kWeightBlockDns must outweigh
// kWeightPermitLocalNetwork, or DNS queries to the router's resolver would be
// permitted along with the rest of the LAN and leak.
constexpr uint8_t kWeightBlockAll = 1;
constexpr uint8_t kWeightPermitLocalNetwork = 8;
constexpr uint8_t kWeightBlockDns = 10;
constexpr uint8_t kWeightPermitInfrastructure = 14;

constexpr uint16_t kDnsPort = 53;

struct Ipv4Prefix {
  uint32_t address;
  uint8_t prefix_length;
};

struct Ipv6Prefix {
  std::array<uint8_t, 16> address;
  uint8_t prefix_length;
};

// Reachable while connected. Note this deliberately includes multicast, which
// covers mDNS (224.0.0.251), LLMNR (224.0.0.252) and SSDP (239.255.255.250), so
// discovering a printer or NAS by name keeps working. Those all use ports other
// than 53, so the DNS block below does not interfere with them.
constexpr Ipv4Prefix kLocalIpv4Prefixes[] = {
    {0x0a000000, 8},   // 10.0.0.0/8, RFC1918.
    {0xac100000, 12},  // 172.16.0.0/12, RFC1918.
    {0xc0a80000, 16},  // 192.168.0.0/16, RFC1918.
    {0xa9fe0000, 16},  // 169.254.0.0/16, link-local.
    {0xe0000000, 4},   // 224.0.0.0/4, multicast.
    {0xffffffff, 32},  // 255.255.255.255, broadcast, which is also DHCP.
};

constexpr Ipv6Prefix kLocalIpv6Prefixes[] = {
    // fc00::/7, unique local addresses.
    {std::array<uint8_t, 16>{0xfc}, 7},
    // fe80::/10, link-local, which is also NDP and DHCPv6.
    {std::array<uint8_t, 16>{0xfe, 0x80}, 10},
    // ff00::/8, multicast.
    {std::array<uint8_t, 16>{0xff}, 8},
};

std::wstring GetFirewallFilterName() {
  switch (chrome::GetChannel()) {
    case version_info::Channel::CANARY:
      return L"Brave VPN Nightly WireGuard Firewall";
    case version_info::Channel::DEV:
      return L"Brave VPN Dev WireGuard Firewall";
    case version_info::Channel::BETA:
      return L"Brave VPN Beta WireGuard Firewall";
    case version_info::Channel::STABLE:
      return L"Brave VPN WireGuard Firewall";
    case version_info::Channel::UNKNOWN:
      return L"Brave VPN Development WireGuard Firewall";
  }

  NOTREACHED();
}

GUID GetFirewallSublayerGUID() {
  switch (chrome::GetChannel()) {
    case version_info::Channel::CANARY:
      // 7edf1bd3-9515-4029-b05d-84810d87335d
      return {0x7edf1bd3,
              0x9515,
              0x4029,
              {0xb0, 0x5d, 0x84, 0x81, 0x0d, 0x87, 0x33, 0x5d}};
    case version_info::Channel::DEV:
      // b18adb98-4ed2-430d-84dc-5159f5ecd961
      return {0xb18adb98,
              0x4ed2,
              0x430d,
              {0x84, 0xdc, 0x51, 0x59, 0xf5, 0xec, 0xd9, 0x61}};
    case version_info::Channel::BETA:
      // b6580eb6-3a2e-4198-bf48-c4a4928867e4
      return {0xb6580eb6,
              0x3a2e,
              0x4198,
              {0xbf, 0x48, 0xc4, 0xa4, 0x92, 0x88, 0x67, 0xe4}};
    case version_info::Channel::STABLE:
      // 78128c6d-f6b0-4abc-9599-841580081911
      return {0x78128c6d,
              0xf6b0,
              0x4abc,
              {0x95, 0x99, 0x84, 0x15, 0x80, 0x08, 0x19, 0x11}};
    case version_info::Channel::UNKNOWN:
      // d0e578c0-6f14-4fd7-8021-ab422f0de990
      return {0xd0e578c0,
              0x6f14,
              0x4fd7,
              {0x80, 0x21, 0xab, 0x42, 0x2f, 0x0d, 0xe9, 0x90}};
  }

  NOTREACHED();
}

std::vector<GUID> GetOutboundLayers() {
  return {FWPM_LAYER_ALE_AUTH_CONNECT_V4, FWPM_LAYER_ALE_AUTH_CONNECT_V6};
}

std::vector<GUID> GetAllLayers() {
  return {FWPM_LAYER_ALE_AUTH_CONNECT_V4, FWPM_LAYER_ALE_AUTH_CONNECT_V6,
          FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
          FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6};
}

DWORD AddFilter(HANDLE engine,
                const GUID& layer,
                FWP_ACTION_TYPE action,
                uint8_t weight,
                base::span<const FWPM_FILTER_CONDITION0> conditions,
                wchar_t* name) {
  FWPM_FILTER0 filter = {};
  filter.subLayerKey = GetFirewallSublayerGUID();
  filter.displayData.name = name;
  filter.displayData.description = name;
  filter.layerKey = layer;
  filter.action.type = action;
  filter.weight.type = FWP_UINT8;
  filter.weight.uint8 = weight;
  filter.filterCondition =
      const_cast<FWPM_FILTER_CONDITION0*>(conditions.data());
  filter.numFilterConditions = static_cast<UINT32>(conditions.size());

  UINT64 filter_id = 0;
  auto result = FwpmFilterAdd0(engine, &filter, nullptr, &filter_id);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "FwpmFilterAdd0 failed, weight: " << static_cast<int>(weight)
            << ", error: " << std::hex << result;
  }
  return result;
}

DWORD AddFilterToLayers(HANDLE engine,
                        const std::vector<GUID>& layers,
                        FWP_ACTION_TYPE action,
                        uint8_t weight,
                        base::span<const FWPM_FILTER_CONDITION0> conditions,
                        wchar_t* name) {
  for (const auto& layer : layers) {
    auto result = AddFilter(engine, layer, action, weight, conditions, name);
    if (result != ERROR_SUCCESS) {
      return result;
    }
  }
  return ERROR_SUCCESS;
}

// Blocks everything that no higher weighted permit matched. This is the
// kill-switch.
DWORD AddBlockAll(HANDLE engine, wchar_t* name) {
  return AddFilterToLayers(engine, GetAllLayers(), FWP_ACTION_BLOCK,
                           kWeightBlockAll, {}, name);
}

// Permits the tunnel service itself so the WireGuard handshake with the VPN
// endpoint can go out over the physical adapter.
DWORD AddPermitTunnelService(HANDLE engine, wchar_t* name) {
  const base::FilePath exe_path = base::PathService::CheckedGet(base::FILE_EXE);

  FWP_BYTE_BLOB* app_id = nullptr;
  auto result = FwpmGetAppIdFromFileName0(exe_path.value().c_str(), &app_id);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "FwpmGetAppIdFromFileName0 failed, error: " << std::hex
            << result;
    return result;
  }

  const std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_ALE_APP_ID,
                             FWP_MATCH_EQUAL,
                             {FWP_BYTE_BLOB_TYPE, {.byteBlob = app_id}}}};
  result = AddFilterToLayers(engine, GetAllLayers(), FWP_ACTION_PERMIT,
                             kWeightPermitInfrastructure, conditions, name);
  FwpmFreeMemory0(reinterpret_cast<void**>(&app_id));
  return result;
}

DWORD AddPermitLoopback(HANDLE engine, wchar_t* name) {
  const std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {
      FWPM_FILTER_CONDITION0{
          FWPM_CONDITION_FLAGS,
          FWP_MATCH_FLAGS_ALL_SET,
          {FWP_UINT32, {.uint32 = FWP_CONDITION_FLAG_IS_LOOPBACK}}}};
  return AddFilterToLayers(engine, GetAllLayers(), FWP_ACTION_PERMIT,
                           kWeightPermitInfrastructure, conditions, name);
}

// Permits everything on the tunnel adapter, i.e. all the traffic that is
// actually being tunneled.
DWORD AddPermitTunnelInterface(HANDLE engine,
                               const NET_LUID& tunnel_luid,
                               wchar_t* name) {
  // WFP stores a pointer for FWP_UINT64, so this must outlive the filter adds.
  UINT64 luid_value = tunnel_luid.Value;
  const std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_LOCAL_INTERFACE,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT64, {.uint64 = &luid_value}}}};
  return AddFilterToLayers(engine, GetAllLayers(), FWP_ACTION_PERMIT,
                           kWeightPermitInfrastructure, conditions, name);
}

// Replaces tunnel.dll's blockDNS filter. Queries that go through the tunnel are
// already permitted at a higher weight by the tunnel interface filter above, so
// this only catches queries that would otherwise leave over another adapter --
// including the ones Windows' multihomed name resolution would send to the
// router alongside the tunnel's resolver.
DWORD AddBlockDns(HANDLE engine, wchar_t* name) {
  const std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_REMOTE_PORT,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT16, {.uint16 = kDnsPort}}}};
  return AddFilterToLayers(engine, GetOutboundLayers(), FWP_ACTION_BLOCK,
                           kWeightBlockDns, conditions, name);
}

DWORD AddPermitLocalNetwork(HANDLE engine, wchar_t* name) {
  const std::vector<GUID> v4_layers = {FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                                       FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4};
  for (const auto& prefix : kLocalIpv4Prefixes) {
    // WFP stores a pointer for FWP_V4_ADDR_MASK, so this must outlive the add.
    FWP_V4_ADDR_AND_MASK addr_and_mask = {};
    addr_and_mask.addr = prefix.address;
    addr_and_mask.mask = ~((1u << (32 - prefix.prefix_length)) - 1u);

    const std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {
        FWPM_FILTER_CONDITION0{
            FWPM_CONDITION_IP_REMOTE_ADDRESS,
            FWP_MATCH_EQUAL,
            {FWP_V4_ADDR_MASK, {.v4AddrMask = &addr_and_mask}}}};
    auto result =
        AddFilterToLayers(engine, v4_layers, FWP_ACTION_PERMIT,
                          kWeightPermitLocalNetwork, conditions, name);
    if (result != ERROR_SUCCESS) {
      return result;
    }
  }

  const std::vector<GUID> v6_layers = {FWPM_LAYER_ALE_AUTH_CONNECT_V6,
                                       FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6};
  for (const auto& prefix : kLocalIpv6Prefixes) {
    FWP_V6_ADDR_AND_MASK addr_and_mask = {};
    base::span(addr_and_mask.addr).copy_from(prefix.address);
    addr_and_mask.prefixLength = prefix.prefix_length;

    const std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {
        FWPM_FILTER_CONDITION0{
            FWPM_CONDITION_IP_REMOTE_ADDRESS,
            FWP_MATCH_EQUAL,
            {FWP_V6_ADDR_MASK, {.v6AddrMask = &addr_and_mask}}}};
    auto result =
        AddFilterToLayers(engine, v6_layers, FWP_ACTION_PERMIT,
                          kWeightPermitLocalNetwork, conditions, name);
    if (result != ERROR_SUCCESS) {
      return result;
    }
  }

  return ERROR_SUCCESS;
}

DWORD AddSublayer(HANDLE engine, wchar_t* name) {
  FWPM_SUBLAYER0 sublayer = {};
  sublayer.subLayerKey = GetFirewallSublayerGUID();
  sublayer.displayData.name = name;
  sublayer.displayData.description = name;
  sublayer.weight = 0xFFFF;
  return FwpmSubLayerAdd0(engine, &sublayer, nullptr);
}

bool AddAllFilters(HANDLE engine, const NET_LUID& tunnel_luid) {
  std::wstring name = GetFirewallFilterName();
  return AddSublayer(engine, name.data()) == ERROR_SUCCESS &&
         AddPermitTunnelService(engine, name.data()) == ERROR_SUCCESS &&
         AddPermitLoopback(engine, name.data()) == ERROR_SUCCESS &&
         AddPermitTunnelInterface(engine, tunnel_luid, name.data()) ==
             ERROR_SUCCESS &&
         AddBlockDns(engine, name.data()) == ERROR_SUCCESS &&
         AddPermitLocalNetwork(engine, name.data()) == ERROR_SUCCESS &&
         AddBlockAll(engine, name.data()) == ERROR_SUCCESS;
}

}  // namespace

// static
std::unique_ptr<ScopedWireguardFirewall> ScopedWireguardFirewall::Create(
    const NET_LUID& tunnel_luid) {
  // A dynamic session means the kernel drops the sublayer and every filter in
  // it when `engine` is closed or this process dies, so we can never leave a
  // machine firewalled off after a crash.
  FWPM_SESSION0 session = {.flags = FWPM_SESSION_FLAG_DYNAMIC};
  HANDLE engine = nullptr;
  auto result =
      FwpmEngineOpen0(nullptr, RPC_C_AUTHN_WINNT, nullptr, &session, &engine);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "FwpmEngineOpen0 failed, error: " << std::hex << result;
    return nullptr;
  }

  // Apply the policy as a unit: a partially installed set could either leak
  // traffic or block the tunnel.
  result = FwpmTransactionBegin0(engine, 0);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "FwpmTransactionBegin0 failed, error: " << std::hex << result;
    FwpmEngineClose0(engine);
    return nullptr;
  }

  if (!AddAllFilters(engine, tunnel_luid)) {
    FwpmTransactionAbort0(engine);
    FwpmEngineClose0(engine);
    return nullptr;
  }

  result = FwpmTransactionCommit0(engine);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "FwpmTransactionCommit0 failed, error: " << std::hex << result;
    FwpmEngineClose0(engine);
    return nullptr;
  }

  VLOG(1) << "WireGuard firewall installed";
  return base::WrapUnique(new ScopedWireguardFirewall(engine));
}

ScopedWireguardFirewall::ScopedWireguardFirewall(HANDLE engine)
    : engine_(engine) {}

ScopedWireguardFirewall::~ScopedWireguardFirewall() {
  if (!engine_) {
    return;
  }
  auto result = FwpmEngineClose0(engine_);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "FwpmEngineClose0 failed, error: " << std::hex << result;
  }
}

class TunnelInterfaceWatcher::State {
 public:
  State(const std::wstring& adapter_name, ReadyCallback on_ready)
      : adapter_name_(adapter_name), on_ready_(std::move(on_ready)) {}

  State(const State&) = delete;
  State& operator=(const State&) = delete;

  ~State() {
    if (notification_) {
      // Waits for any in-flight callback to return, so it is safe to tear the
      // rest of this object down afterwards.
      CancelMibChangeNotify2(notification_);
    }
  }

  bool Start() {
    // InitialNotification so we also handle an adapter that came up before the
    // subscription was registered.
    auto result =
        NotifyIpInterfaceChange(AF_UNSPEC, &State::OnInterfaceChange, this,
                                /*InitialNotification=*/TRUE, &notification_);
    if (result != NO_ERROR) {
      VLOG(1) << "NotifyIpInterfaceChange failed, error: " << std::hex
              << result;
      return false;
    }
    return true;
  }

 private:
  static void CALLBACK OnInterfaceChange(void* context,
                                         MIB_IPINTERFACE_ROW* row,
                                         MIB_NOTIFICATION_TYPE type) {
    static_cast<State*>(context)->OnInterfaceChangeImpl(type);
  }

  void OnInterfaceChangeImpl(MIB_NOTIFICATION_TYPE type) {
    if (type == MibDeleteInstance) {
      return;
    }

    base::AutoLock auto_lock(lock_);
    if (on_ready_.is_null()) {
      return;
    }
    NET_LUID luid = {};
    auto result = ConvertInterfaceAliasToLuid(adapter_name_.c_str(), &luid);
    if (result != NO_ERROR) {
      // Expected until tunnel.dll creates the adapter. If it never resolves,
      // the adapter is not named what we expect and the firewall is never
      // installed, so this is worth having in the log.
      VLOG(2) << "Tunnel adapter " << adapter_name_
              << " not present yet, error: " << std::hex << result;
      return;
    }
    std::move(on_ready_).Run(luid);
  }

  const std::wstring adapter_name_;
  base::Lock lock_;
  ReadyCallback on_ready_ GUARDED_BY(lock_);
  HANDLE notification_ = nullptr;
};

// static
std::unique_ptr<TunnelInterfaceWatcher> TunnelInterfaceWatcher::Create(
    const std::wstring& adapter_name,
    ReadyCallback on_ready) {
  auto state = std::make_unique<State>(adapter_name, std::move(on_ready));
  if (!state->Start()) {
    return nullptr;
  }
  return base::WrapUnique(new TunnelInterfaceWatcher(std::move(state)));
}

TunnelInterfaceWatcher::TunnelInterfaceWatcher(std::unique_ptr<State> state)
    : state_(std::move(state)) {}

TunnelInterfaceWatcher::~TunnelInterfaceWatcher() = default;

std::wstring GetTunnelInterfaceAlias(const base::FilePath& config) {
  // tunnel.dll derives the adapter name from the config file name, minus the
  // `.conf` suffix.
  return config.BaseName().RemoveExtension().value();
}

void RequestTunnelShutdown() {
  // Called from the interface change callback. ControlService() returns once
  // tunnel.dll's control handler has moved the service to STOP_PENDING rather
  // than waiting for the tunnel to be fully down, so this does not deadlock
  // against CancelMibChangeNotify2() during teardown.
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
  if (!scm.is_valid()) {
    VLOG(1) << "::OpenSCManager failed, error: " << std::hex
            << ::GetLastError();
    return;
  }
  ScopedScHandle service(
      ::OpenService(scm.Get(), GetBraveVpnWireguardTunnelServiceName().c_str(),
                    SERVICE_STOP));
  if (!service.is_valid()) {
    VLOG(1) << "::OpenService failed, error: " << std::hex << ::GetLastError();
    return;
  }
  SERVICE_STATUS status = {};
  if (!::ControlService(service.Get(), SERVICE_CONTROL_STOP, &status)) {
    VLOG(1) << "::ControlService failed to stop the tunnel, error: " << std::hex
            << ::GetLastError();
  }
}

}  // namespace wireguard

}  // namespace brave_vpn
