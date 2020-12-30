/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_request.h"

namespace permissions {
namespace {

std::string GetPermissionRequestString(PermissionRequestType type);
std::string GetPermissionRequestString_ChromiumImpl(PermissionRequestType type);

}  // namespace
}  // namespace permissions

#include "../../../../components/permissions/permission_uma_util.cc"

namespace permissions {
namespace {

std::string GetPermissionRequestString(PermissionRequestType type) {
  if (type == PermissionRequestType::PERMISSION_WIDEVINE)
    return "Widevine";
  if (type == PermissionRequestType::PERMISSION_WALLET)
    return "Wallet";
  return GetPermissionRequestString_ChromiumImpl(type);
}

}  // namespace
}  // namespace permissions

