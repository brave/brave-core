// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
#ifndef BRAVE_EMBEDDED_TOR_H_
#define BRAVE_EMBEDDED_TOR_H_

#include <stddef.h>

typedef struct brave_tor_host brave_tor_host;

/// Copies argv. Returns NULL if Tor is already active or startup fails.
/// The returned control descriptor is private to this host, not a TCP port.
brave_tor_host* _Nullable brave_tor_host_start(
    int argc,
    const char* _Nonnull const* _Nonnull argv,
    int* _Nonnull control_descriptor);

/// Closes the owning control socket and joins Tor on a detached cleanup thread.
/// The descriptor must not be used after this call.
void brave_tor_host_release(brave_tor_host* _Nullable host);

#endif  // BRAVE_EMBEDDED_TOR_H_
