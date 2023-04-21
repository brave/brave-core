/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/crash/core/app/crashpad.h"

#if BUILDFLAG(IS_WIN)
namespace {
// Split into two places to avoid patching:
// components\brave_vpn\browser\connection\win\brave_vpn_helper\brave_vpn_helper_crash_reporter_client.cc
// // NOLINT Need keep it in sync
constexpr char kBraveVPNHelperProcessType[] = "brave-vpn-helper";
constexpr char kBraveWireguardProcessType[] = "brave-wireguard-vpn";
}  // namespace

#define BRAVE_INITIALIZE_CRASHPAD_IMPL_PROCESS_TYPE \
  process_type == kBraveVPNHelperProcessType ||     \
      process_type == kBraveWireguardProcessType ||
#endif

#include "src/components/crash/core/app/crashpad.cc"
#if BUILDFLAG(IS_WIN)
#undef BRAVE_INITIALIZE_CRASHPAD_IMPL_PROCESS_TYPE
#endif
