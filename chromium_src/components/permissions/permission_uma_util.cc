/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_uma_util.h"

#include "build/build_config.h"

namespace permissions {
namespace {

std::string GetPermissionRequestString(RequestTypeForUma type);
std::string GetPermissionRequestString_ChromiumImpl(RequestTypeForUma type);

}  // namespace
}  // namespace permissions

#if !defined(OS_ANDROID) && !defined(OS_IOS)
#define BRAVE_GET_UMA_VALUE_FOR_REQUEST_TYPE \
  case RequestType::kWidevine:               \
    return RequestTypeForUma::PERMISSION_WIDEVINE;
#else
#define BRAVE_GET_UMA_VALUE_FOR_REQUEST_TYPE
#endif

#include "../../../../components/permissions/permission_uma_util.cc"
#undef BRAVE_GET_UMA_VALUE_FOR_REQUEST_TYPE

namespace permissions {
namespace {

std::string GetPermissionRequestString(RequestTypeForUma type) {
  if (type == RequestTypeForUma::PERMISSION_WIDEVINE)
    return "Widevine";
  if (type == RequestTypeForUma::PERMISSION_WALLET)
    return "Wallet";
  return GetPermissionRequestString_ChromiumImpl(type);
}

}  // namespace
}  // namespace permissions

