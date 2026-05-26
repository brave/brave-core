/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/crash/core/app/crashpad.h"

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
namespace {
#if BUILDFLAG(IS_WIN)
// Split into two places to avoid patching:
// NOLINTNEXTLINE
// components\brave_vpn\browser\connection\win\brave_vpn_helper\brave_vpn_helper_crash_reporter_client.cc
// Need keep it in sync
constexpr char kBraveVPNHelperProcessType[] = "brave-vpn-helper";

// Split into two places to avoid patching:
// NOLINTNEXTLINE
// components\brave_vpn\browser\connection\wireguard\win\brave_vpn_wireguard_service\brave_wireguard_service_crash_reporter_client.cc
// Need keep it in sync
constexpr char kBraveWireguardProcessType[] = "brave-vpn-wireguard-service";
#endif  // BUILDFLAG(IS_WIN)

// Split into two places to avoid patching:
// components/brave_vpn/v2/app/agent/agent_app.cc
// Need keep it in sync
constexpr char kBraveVpnAgentAppProcessType[] = "brave-vpn-agent-app";
}  // namespace

#if BUILDFLAG(IS_WIN)
#define BRAVE_INITIALIZE_CRASHPAD_IMPL_PROCESS_TYPE \
  process_type == kBraveVPNHelperProcessType ||     \
      process_type == kBraveWireguardProcessType || \
      process_type == kBraveVpnAgentAppProcessType ||
#else  // BUILDFLAG(IS_WIN)
#define BRAVE_INITIALIZE_CRASHPAD_IMPL_PROCESS_TYPE \
  process_type == kBraveVpnAgentAppProcessType ||
#endif  // BUILDFLAG(IS_WIN)
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)

#include <components/crash/core/app/crashpad.cc>

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#undef BRAVE_INITIALIZE_CRASHPAD_IMPL_PROCESS_TYPE
#endif
