/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/pref_names.h"

// Used to determine which method should be used to resolve ipfs:// and ipns:///
// schemes, between:
// Ask: Uses a gateway but also prompts the user with an infobar.
// Gateway: Uses a gateway without prompting the user.
// Local: Uses a local node.
// Disabled: Disables all IPFS handling.
const char kIPFSResolveMethod[] = "brave.ipfs.resolve_method";

// Stores the location of the IPFS binary to determine if a local node was ever
// installed.
const char kIPFSBinaryAvailable[] = "brave.ipfs.binary_available";

// Used to determine whether to automatically fallback to gateway when the
// local node is not available.
const char kIPFSAutoFallbackToGateway[] = "brave.ipfs.auto_fallback_to_gateway";
