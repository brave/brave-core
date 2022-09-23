/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// NOLINT(build/header_guard)
// no-include-guard-because-multiply-included

#include "src/net/base/net_error_list.h"

// Error occurs when user tries to access ipfs sites in
// incognito context
NET_ERROR(INCOGNITO_IPFS_NOT_ALLOWED, -10001)
NET_ERROR(IPFS_DISABLED, -10002)
NET_ERROR(IPFS_RESOLVE_METHOD_NOT_SELECTED, -10003)

NET_ERROR(ENS_OFFCHAIN_LOOKUP_NOT_SELECTED, -10004)
