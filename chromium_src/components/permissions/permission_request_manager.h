/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_MANAGER_H_

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#define set_view_factory_for_testing                                           \
  AcceptDenyCancel(const std::vector<PermissionRequest*>& accepted_requests,   \
                   const std::vector<PermissionRequest*>& denied_requests,     \
                   const std::vector<PermissionRequest*>& cancelled_requests); \
  bool ShouldGroupRequests(PermissionRequest* a, PermissionRequest* b);        \
  void set_view_factory_for_testing
#endif  // BUILDFLAG(BRAVE_WALLET_ENABLED)

#include "../../../../components/permissions/permission_request_manager.h"

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#undef set_view_factory_for_testing
#endif

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_MANAGER_H_
