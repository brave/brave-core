/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// IsOnionHostnameResolutionBlocked() is called from the plasters in
// HostResolverManager::ResolveLocally() and StartBootstrapFollowup().
#include "brave/net/dns/onion_hostname_resolution.h"

#include <net/dns/host_resolver_manager.cc>
