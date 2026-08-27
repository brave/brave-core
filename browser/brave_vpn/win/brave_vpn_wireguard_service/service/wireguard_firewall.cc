/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_firewall.h"

#include <fwpmu.h>

#include <array>
#include <ios>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/rand_util.h"
#include "base/scoped_generic.h"
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"
#include "brave/browser/brave_vpn/win/service_details.h"
#include "brave/components/brave_vpn/common/win/scoped_sc_handle.h"

namespace brave_vpn::wireguard {

namespace {

// Filter weights within our sublayer. A packet typically matches several of
// these at once -- a DNS query to the router matches the block-all, the local
// network permit and the DNS block -- and WFP lets the highest weight decide.
// These numbers are therefore the policy itself, not a hint, and the
// static_asserts on these constants spell out the orderings it depends on.
//
// Ties are broken in an unspecified order, so no block and permit that can
// match the same traffic may share a weight. The gaps leave room to slot a
// filter in without renumbering.
constexpr uint8_t kWeightBlockAll = 1;
constexpr uint8_t kWeightPermitLocalNetwork = 8;
constexpr uint8_t kWeightBlockDns = 10;
// Only exists until the tunnel is up. See AddTemporaryPermitDns(); it is
// deleted in the same transaction that adds the DNS block, so the two never
// coexist and their relative weight does not matter.
constexpr uint8_t kWeightPermitTemporaryDns = 12;
constexpr uint8_t kWeightPermitInfrastructure = 14;

static_assert(kWeightBlockAll < kWeightPermitLocalNetwork &&
                  kWeightBlockAll < kWeightPermitTemporaryDns &&
                  kWeightBlockAll < kWeightPermitInfrastructure,
              "block-all is the floor: a permit that does not outweigh it "
              "never takes effect and that traffic silently disappears.");

static_assert(kWeightPermitLocalNetwork < kWeightBlockDns,
              "a query to the router's resolver matches the local network "
              "permit as well, so the DNS block has to win or DNS leaks out "
              "of the tunnel.");

static_assert(kWeightBlockDns < kWeightPermitInfrastructure,
              "DNS through the tunnel has to keep working, so the tunnel "
              "interface permit has to outweigh the DNS block in turn.");

struct FwpmMemoryTraits {
  static void* InvalidValue() { return nullptr; }
  static void Free(void* memory) {
    // FwpmFreeMemory0() nulls the pointer it is handed, which is why it takes
    // a void**. Passing a local copy frees the same allocation without
    // type-punning a typed T** into a void**.
    FwpmFreeMemory0(&memory);
  }
};
using ScopedFwpmMemory = base::ScopedGeneric<void*, FwpmMemoryTraits>;

constexpr uint8_t kUdpProtocol = IPPROTO_UDP;
constexpr uint16_t kDhcpV4ClientPort = 68;
constexpr uint16_t kDhcpV4ServerPort = 67;
constexpr uint16_t kDhcpV6ClientPort = 546;
constexpr uint16_t kDhcpV6ServerPort = 547;
constexpr uint32_t kIpv4Broadcast = 0xffffffff;

struct Ipv4Prefix {
  uint32_t address;
  uint8_t prefix_length;
};

struct Ipv6Prefix {
  std::array<uint8_t, 16> address;
  uint8_t prefix_length;
};

// Reachable while connected. Note this includes multicast for general local
// network discovery (such as SSDP on 239.255.255.250). However, multicast
// name resolution protocols (mDNS on 224.0.0.251, LLMNR on 224.0.0.252) are
// explicitly overridden and blocked by AddBlockDns() to prevent cleartext
// DNS leaks. As a result, discovering a printer or NAS by local hostname is
// intentionally disabled, though direct IP access remains functional.
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

constexpr bool AreValidPrefixLengths() {
  for (const auto& prefix : kLocalIpv4Prefixes) {
    if (prefix.prefix_length == 0 || prefix.prefix_length > 32) {
      return false;
    }
  }
  for (const auto& prefix : kLocalIpv6Prefixes) {
    if (prefix.prefix_length == 0 || prefix.prefix_length > 128) {
      return false;
    }
  }
  return true;
}

static_assert(AreValidPrefixLengths(),
              "A local network prefix must be a real subnet: a /0 entry would "
              "match every address, so the permit below would outweigh the "
              "block-all filter and defeat the kill-switch.");

// Prefix length to the netmask WFP wants, in host byte order. Defined across
// the whole 0..32 range so that a bad table entry is a wrong filter rather than
// undefined behavior; AreValidPrefixLengths() is what rejects bad entries.
constexpr uint32_t Ipv4Netmask(uint8_t prefix_length) {
  return prefix_length == 0 ? 0u : 0xffffffffu << (32u - prefix_length);
}

// The provider owns everything we install and exists so that a `netsh wfp show
// filters` dump can be attributed to Brave at a glance; the sublayer holds the
// filters themselves.
struct BaseObjects {
  GUID provider;
  GUID sublayer;
};

// Fresh keys for every run. Both objects are dynamic and we never look either
// one up again, so neither needs a stable identity -- and a fixed key would
// collide with the objects of a previous run that the filtering engine has not
// reaped yet, which is exactly what a crash-restart looks like. tunnel.dll does
// the same with its own base objects.
GUID CreateTransientKey() {
  GUID key = {};
  base::RandBytes(base::byte_span_from_ref(key));
  // Mark it as a random (v4) UUID so it reads as one wherever it is dumped.
  key.Data3 = (key.Data3 & 0x0fff) | 0x4000;
  key.Data4[0] = (key.Data4[0] & 0x3f) | 0x80;
  return key;
}

BaseObjects CreateBaseObjects() {
  return {.provider = CreateTransientKey(), .sublayer = CreateTransientKey()};
}

// Not constexpr: the FWPM_LAYER_* GUIDs are extern symbols resolved at link
// time. Returning by value is still allocation-free and converts to a span.
std::array<GUID, 2u> GetOutboundLayers() {
  return {FWPM_LAYER_ALE_AUTH_CONNECT_V4, FWPM_LAYER_ALE_AUTH_CONNECT_V6};
}

std::array<GUID, 4u> GetAllLayers() {
  return {FWPM_LAYER_ALE_AUTH_CONNECT_V4, FWPM_LAYER_ALE_AUTH_CONNECT_V6,
          FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
          FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6};
}

DWORD AddFilter(HANDLE engine,
                const BaseObjects& base_objects,
                const GUID& layer,
                FWP_ACTION_TYPE action,
                uint8_t weight,
                base::span<FWPM_FILTER_CONDITION0> conditions,
                const wchar_t* name,
                UINT64* out_filter_id = nullptr) {
  // FWPM_FILTER0 holds non-const pointers even though WFP only reads through
  // them, so point it at mutable copies of our own rather than casting away
  // constness we actually rely on.
  GUID provider_key = base_objects.provider;
  std::wstring display_name(name);

  FWPM_FILTER0 filter = {};
  filter.subLayerKey = base_objects.sublayer;
  filter.providerKey = &provider_key;
  filter.displayData.name = display_name.data();
  filter.displayData.description = display_name.data();
  filter.layerKey = layer;
  filter.action.type = action;
  filter.weight.type = FWP_UINT8;
  filter.weight.uint8 = weight;
  filter.filterCondition = conditions.data();
  filter.numFilterConditions = static_cast<UINT32>(conditions.size());

  UINT64 filter_id = 0;
  auto result = FwpmFilterAdd0(engine, &filter, nullptr, &filter_id);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "FwpmFilterAdd0 failed, weight: " << static_cast<int>(weight)
            << ", error: " << std::hex << result;
    return result;
  }
  if (out_filter_id) {
    *out_filter_id = filter_id;
  }
  return result;
}

DWORD AddFilterToLayers(HANDLE engine,
                        const BaseObjects& base_objects,
                        base::span<const GUID> layers,
                        FWP_ACTION_TYPE action,
                        uint8_t weight,
                        base::span<FWPM_FILTER_CONDITION0> conditions,
                        const wchar_t* name) {
  for (const auto& layer : layers) {
    auto result = AddFilter(engine, base_objects, layer, action, weight,
                            conditions, name);
    if (result != ERROR_SUCCESS) {
      return result;
    }
  }
  return ERROR_SUCCESS;
}

// Blocks everything that no higher weighted permit matched. This is the
// kill-switch.
DWORD AddBlockAll(HANDLE engine, const BaseObjects& base_objects) {
  return AddFilterToLayers(engine, base_objects, GetAllLayers(),
                           FWP_ACTION_BLOCK, kWeightBlockAll, {}, L"Block all");
}

// Permits the tunnel service itself so the WireGuard handshake with the VPN
// endpoint can go out over the physical adapter.
DWORD AddPermitTunnelService(HANDLE engine, const BaseObjects& base_objects) {
  const base::FilePath exe_path = base::PathService::CheckedGet(base::FILE_EXE);

  FWP_BYTE_BLOB* app_id = nullptr;
  auto result = FwpmGetAppIdFromFileName0(exe_path.value().c_str(), &app_id);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "FwpmGetAppIdFromFileName0 failed, error: " << std::hex
            << result;
    return result;
  }
  ScopedFwpmMemory scoped_app_id(app_id);

  std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_ALE_APP_ID,
                             FWP_MATCH_EQUAL,
                             {FWP_BYTE_BLOB_TYPE, {.byteBlob = app_id}}}};
  return AddFilterToLayers(engine, base_objects, GetAllLayers(),
                           FWP_ACTION_PERMIT, kWeightPermitInfrastructure,
                           conditions, L"Permit Brave VPN service");
}

// DHCP has to keep working while the tunnel is up, or the machine silently
// loses its lease. The local network permits cover a server on a private
// address, but not the responses from one that answers from a public address,
// so these match on the DHCP port pairs instead.
//
// Mirrors tunnel.dll's own permitDHCPIPv4/permitDHCPIPv6: outbound is
// restricted to the broadcast address rather than allowing any destination on
// port 67, because nothing stops another local process from binding port 68 on
// Windows. A unicast renewal that goes unanswered falls back to a broadcast
// rebind, which this covers.
DWORD AddPermitDhcp(HANDLE engine, const BaseObjects& base_objects) {
  std::array<FWPM_FILTER_CONDITION0, 4u> v4_request = {
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_PROTOCOL,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT8, {.uint8 = kUdpProtocol}}},
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_LOCAL_PORT,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT16, {.uint16 = kDhcpV4ClientPort}}},
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_REMOTE_PORT,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT16, {.uint16 = kDhcpV4ServerPort}}},
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_REMOTE_ADDRESS,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT32, {.uint32 = kIpv4Broadcast}}}};
  auto result = AddFilter(engine, base_objects, FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                          FWP_ACTION_PERMIT, kWeightPermitInfrastructure,
                          v4_request, L"Permit DHCP request");
  if (result != ERROR_SUCCESS) {
    return result;
  }

  // No address condition: the offer or ack comes from whatever address the
  // server or relay answers from, which is the case the local network permits
  // miss.
  std::array<FWPM_FILTER_CONDITION0, 3u> v4_response = {
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_PROTOCOL,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT8, {.uint8 = kUdpProtocol}}},
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_LOCAL_PORT,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT16, {.uint16 = kDhcpV4ClientPort}}},
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_REMOTE_PORT,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT16, {.uint16 = kDhcpV4ServerPort}}}};
  result = AddFilter(engine, base_objects, FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                     FWP_ACTION_PERMIT, kWeightPermitInfrastructure,
                     v4_response, L"Permit DHCP response");
  if (result != ERROR_SUCCESS) {
    return result;
  }

  // DHCPv6 only ever talks to multicast or link-local addresses, which the
  // local network permits already cover, so the port pair is all that is left
  // to pin down.
  std::array<FWPM_FILTER_CONDITION0, 3u> v6 = {
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_PROTOCOL,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT8, {.uint8 = kUdpProtocol}}},
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_LOCAL_PORT,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT16, {.uint16 = kDhcpV6ClientPort}}},
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_REMOTE_PORT,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT16, {.uint16 = kDhcpV6ServerPort}}}};
  const std::array<GUID, 2u> v6_layers = {FWPM_LAYER_ALE_AUTH_CONNECT_V6,
                                          FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6};
  return AddFilterToLayers(engine, base_objects, v6_layers, FWP_ACTION_PERMIT,
                           kWeightPermitInfrastructure, v6, L"Permit DHCPv6");
}

DWORD AddPermitLoopback(HANDLE engine, const BaseObjects& base_objects) {
  std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {FWPM_FILTER_CONDITION0{
      FWPM_CONDITION_FLAGS,
      FWP_MATCH_FLAGS_ALL_SET,
      {FWP_UINT32, {.uint32 = FWP_CONDITION_FLAG_IS_LOOPBACK}}}};
  return AddFilterToLayers(engine, base_objects, GetAllLayers(),
                           FWP_ACTION_PERMIT, kWeightPermitInfrastructure,
                           conditions, L"Permit loopback");
}

// Permits everything on the tunnel adapter, i.e. all the traffic that is
// actually being tunneled.
DWORD AddPermitTunnelInterface(HANDLE engine,
                               const BaseObjects& base_objects,
                               const NET_LUID& tunnel_luid) {
  // WFP stores a pointer for FWP_UINT64, so this must outlive the filter adds.
  UINT64 luid_value = tunnel_luid.Value;
  std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_LOCAL_INTERFACE,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT64, {.uint64 = &luid_value}}}};
  return AddFilterToLayers(engine, base_objects, GetAllLayers(),
                           FWP_ACTION_PERMIT, kWeightPermitInfrastructure,
                           conditions, L"Permit tunnel interface");
}

// Replaces and expands tunnel.dll's blockDNS filter. Queries that go through
// the tunnel are already permitted at a higher weight by
// AddPermitTunnelInterface(), so this only catches queries that would otherwise
// leave over another adapter.
//
// In addition to standard DNS (port 53), this explicitly blocks fallback
// multicast and broadcast name resolution protocols: NetBIOS (137), mDNS
// (5353), and LLMNR (5355). This is critical to prevent cleartext DNS leaks on
// the local segment, which happen when Windows' multihomed name resolution
// broadcasts queries to the LAN after the tunnel's resolver returns NXDOMAIN
// (e.g., when a user types a local hostname).
DWORD AddBlockDns(HANDLE engine, const BaseObjects& base_objects) {
  constexpr uint16_t kNameResolutionPorts[] = {
      53,    // Standard DNS
      137,   // NetBIOS Name Service
      5353,  // mDNS
      5355,  // LLMNR
  };

  for (const uint16_t port : kNameResolutionPorts) {
    std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {
        FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_REMOTE_PORT,
                               FWP_MATCH_EQUAL,
                               {FWP_UINT16, {.uint16 = port}}}};

    auto result = AddFilterToLayers(engine, base_objects, GetOutboundLayers(),
                                    FWP_ACTION_BLOCK, kWeightBlockDns,
                                    conditions, L"Block DNS");
    if (result != ERROR_SUCCESS) {
      return result;
    }
  }
  return ERROR_SUCCESS;
}

DWORD AddPermitLocalNetwork(HANDLE engine, const BaseObjects& base_objects) {
  const std::array<GUID, 2u> v4_layers = {FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                                          FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4};
  for (const auto& prefix : kLocalIpv4Prefixes) {
    // WFP stores a pointer for FWP_V4_ADDR_MASK, so this must outlive the add.
    FWP_V4_ADDR_AND_MASK addr_and_mask = {};
    addr_and_mask.addr = prefix.address;
    addr_and_mask.mask = Ipv4Netmask(prefix.prefix_length);

    std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {FWPM_FILTER_CONDITION0{
        FWPM_CONDITION_IP_REMOTE_ADDRESS,
        FWP_MATCH_EQUAL,
        {FWP_V4_ADDR_MASK, {.v4AddrMask = &addr_and_mask}}}};
    auto result = AddFilterToLayers(
        engine, base_objects, v4_layers, FWP_ACTION_PERMIT,
        kWeightPermitLocalNetwork, conditions, L"Permit local network");
    if (result != ERROR_SUCCESS) {
      return result;
    }
  }

  const std::array<GUID, 2u> v6_layers = {FWPM_LAYER_ALE_AUTH_CONNECT_V6,
                                          FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6};
  for (const auto& prefix : kLocalIpv6Prefixes) {
    FWP_V6_ADDR_AND_MASK addr_and_mask = {};
    base::span(addr_and_mask.addr).copy_from(prefix.address);
    addr_and_mask.prefixLength = prefix.prefix_length;

    std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {FWPM_FILTER_CONDITION0{
        FWPM_CONDITION_IP_REMOTE_ADDRESS,
        FWP_MATCH_EQUAL,
        {FWP_V6_ADDR_MASK, {.v6AddrMask = &addr_and_mask}}}};
    auto result = AddFilterToLayers(
        engine, base_objects, v6_layers, FWP_ACTION_PERMIT,
        kWeightPermitLocalNetwork, conditions, L"Permit local network");
    if (result != ERROR_SUCCESS) {
      return result;
    }
  }

  return ERROR_SUCCESS;
}

// Leaves `serviceName` null on purpose: that field is how BFE knows to start
// the owning service on demand, which makes no sense for a transient provider.
// The provider is what attributes the whole set to Brave in a `netsh wfp show
// filters` dump, since the keys themselves are transient. The filters it owns
// are named for what they do rather than repeating the product name.
DWORD AddProvider(HANDLE engine, const BaseObjects& base_objects) {
  std::wstring name = GetBraveVpnWireguardServiceDisplayName();
  FWPM_PROVIDER0 provider = {};
  provider.providerKey = base_objects.provider;
  provider.displayData.name = name.data();
  provider.displayData.description = name.data();
  return FwpmProviderAdd0(engine, &provider, nullptr);
}

DWORD AddSublayer(HANDLE engine, const BaseObjects& base_objects) {
  std::wstring name = GetBraveVpnWireguardServiceDisplayName();
  GUID provider_key = base_objects.provider;
  FWPM_SUBLAYER0 sublayer = {};
  sublayer.subLayerKey = base_objects.sublayer;
  sublayer.providerKey = &provider_key;
  sublayer.displayData.name = name.data();
  sublayer.displayData.description = name.data();
  sublayer.weight = 0xFFFF;
  return FwpmSubLayerAdd0(engine, &sublayer, nullptr);
}

// Phase one only. tunnel.dll has to resolve the endpoint hostname before there
// is a tunnel to resolve it through, and on Windows that query is issued by the
// DNS Client service rather than by us, so no app-based permit would cover it.
// AddTunnelFilters() withdraws this and hands over to AddBlockDns().
DWORD AddTemporaryPermitDns(HANDLE engine,
                            const BaseObjects& base_objects,
                            std::vector<UINT64>* filter_ids) {
  constexpr uint16_t kDnsPort = 53;

  std::array<FWPM_FILTER_CONDITION0, 1u> conditions = {
      FWPM_FILTER_CONDITION0{FWPM_CONDITION_IP_REMOTE_PORT,
                             FWP_MATCH_EQUAL,
                             {FWP_UINT16, {.uint16 = kDnsPort}}}};
  for (const auto& layer : GetOutboundLayers()) {
    UINT64 filter_id = 0;
    auto result = AddFilter(engine, base_objects, layer, FWP_ACTION_PERMIT,
                            kWeightPermitTemporaryDns, conditions,
                            L"Permit DNS while connecting", &filter_id);
    if (result != ERROR_SUCCESS) {
      return result;
    }
    filter_ids->push_back(filter_id);
  }
  return ERROR_SUCCESS;
}

// Everything that does not need the tunnel adapter, so that block-all is in
// force before the adapter exists rather than after.
bool AddGlobalFilters(HANDLE engine,
                      const BaseObjects& base_objects,
                      std::vector<UINT64>* temporary_dns_filter_ids) {
  return AddProvider(engine, base_objects) == ERROR_SUCCESS &&
         AddSublayer(engine, base_objects) == ERROR_SUCCESS &&
         AddPermitTunnelService(engine, base_objects) == ERROR_SUCCESS &&
         AddPermitLoopback(engine, base_objects) == ERROR_SUCCESS &&
         AddPermitDhcp(engine, base_objects) == ERROR_SUCCESS &&
         AddPermitLocalNetwork(engine, base_objects) == ERROR_SUCCESS &&
         AddTemporaryPermitDns(engine, base_objects,
                               temporary_dns_filter_ids) == ERROR_SUCCESS &&
         AddBlockAll(engine, base_objects) == ERROR_SUCCESS;
}

// The rest, once the adapter is up: let the tunnel carry traffic and pin DNS to
// it, dropping the allowance that kept endpoint resolution working.
bool AddTunnelFilters(HANDLE engine,
                      const BaseObjects& base_objects,
                      const NET_LUID& tunnel_luid,
                      base::span<const UINT64> temporary_dns_filter_ids) {
  return AddPermitTunnelInterface(engine, base_objects, tunnel_luid) ==
             ERROR_SUCCESS &&
         AddBlockDns(engine, base_objects) == ERROR_SUCCESS;
}

// Logs every WFP sublayer that is not ours, to help diagnose a tunnel that
// connects but carries no traffic because another product's kill-switch --
// another Brave channel, another VPN -- is blocking it despite our own
// filters permitting it. WFP arbitrates sublayers independently and then ANDs
// the verdicts, so any one of them voting to block wins regardless of its
// weight; that is why this logs the whole inventory rather than filtering by
// weight.
void LogOtherSublayers(HANDLE engine, const GUID& our_sublayer) {
  HANDLE enum_handle = nullptr;
  auto result = FwpmSubLayerCreateEnumHandle0(engine, nullptr, &enum_handle);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "FwpmSubLayerCreateEnumHandle0 failed, error: " << std::hex
            << result;
    return;
  }

  FWPM_SUBLAYER0** entries = nullptr;
  UINT32 count = 0;
  result =
      FwpmSubLayerEnum0(engine, enum_handle,
                        /*numEntriesRequested=*/0xFFFFFFFF, &entries, &count);
  FwpmSubLayerDestroyEnumHandle0(engine, enum_handle);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "FwpmSubLayerEnum0 failed, error: " << std::hex << result;
    return;
  }
  ScopedFwpmMemory scoped_entries(entries);

  // SAFETY: FwpmSubLayerEnum0() returned ERROR_SUCCESS, which per its contract
  // means it wrote `count` valid entries into `entries`.
  auto sublayers = UNSAFE_BUFFERS(base::span(entries, count));
  for (const FWPM_SUBLAYER0* entry : sublayers) {
    const FWPM_SUBLAYER0& sublayer = *entry;
    if (IsEqualGUID(sublayer.subLayerKey, our_sublayer)) {
      continue;
    }
    VLOG(1) << "Other WFP sublayer present: "
            << (sublayer.displayData.name ? sublayer.displayData.name
                                          : L"(unnamed)")
            << ", weight: " << sublayer.weight;
  }
}

}  // namespace

// static
std::unique_ptr<ScopedWireguardFirewall> ScopedWireguardFirewall::Create() {
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

  const BaseObjects base_objects = CreateBaseObjects();
  std::vector<UINT64> temporary_dns_filter_ids;
  if (!AddGlobalFilters(engine, base_objects, &temporary_dns_filter_ids)) {
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

  VLOG(1) << "WireGuard firewall installed, waiting for the tunnel adapter";
  LogOtherSublayers(engine, base_objects.sublayer);
  return base::WrapUnique(new ScopedWireguardFirewall(
      engine, base_objects.provider, base_objects.sublayer,
      std::move(temporary_dns_filter_ids)));
}

bool ScopedWireguardFirewall::PermitTunnelInterface(
    const NET_LUID& tunnel_luid) {
  const BaseObjects base_objects = {.provider = provider_key_,
                                    .sublayer = sublayer_key_};
  auto result = FwpmTransactionBegin0(engine_, 0);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "FwpmTransactionBegin0 failed, error: " << std::hex << result;
    return false;
  }

  WithdrawTemporaryDns();

  if (!AddTunnelFilters(engine_, base_objects, tunnel_luid,
                        temporary_dns_filter_ids_)) {
    FwpmTransactionAbort0(engine_);
    return false;
  }

  result = FwpmTransactionCommit0(engine_);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "FwpmTransactionCommit0 failed, error: " << std::hex << result;
    return false;
  }

  VLOG(1) << "WireGuard firewall now permits the tunnel adapter";
  return true;
}

void ScopedWireguardFirewall::WithdrawTemporaryDns() {
  for (const auto filter_id : temporary_dns_filter_ids_) {
    // Ignore return values: we are likely already on a failure path,
    // and WFP will safely return an error if the filter is already gone.
    FwpmFilterDeleteById0(engine_, filter_id);
  }

  // Clear the list so subsequent calls (e.g., from the watchdog) are a safe
  // no-op.
  temporary_dns_filter_ids_.clear();
}

ScopedWireguardFirewall::ScopedWireguardFirewall(
    HANDLE engine,
    const GUID& provider_key,
    const GUID& sublayer_key,
    std::vector<UINT64> temporary_dns_filter_ids)
    : engine_(engine),
      provider_key_(provider_key),
      sublayer_key_(sublayer_key),
      temporary_dns_filter_ids_(std::move(temporary_dns_filter_ids)) {}

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

}  // namespace brave_vpn::wireguard
