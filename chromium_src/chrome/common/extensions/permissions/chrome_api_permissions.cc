/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define GetPermissionInfos GetPermissionInfos_ChromiumImpl
#include "../../../../../../chrome/common/extensions/permissions/chrome_api_permissions.cc"
#undef GetPermissionInfos

namespace extensions {
namespace chrome_api_permissions {

namespace {

constexpr APIPermissionInfo::InitInfo brave_permissions_to_register[] = {
    {APIPermissionID::kIpfs, "ipfs",
     APIPermissionInfo::kFlagImpliesFullURLAccess}};

// Merges Brave and Chrormium constant arrays to final list of permissions.
template <typename T, size_t N>
class PermissionsContainer {
 public:
  constexpr PermissionsContainer(base::span<const T> chromium,
                                 base::span<const T> brave) {
    CHECK(N == chromium.size() + brave.size());
    size_t last_index = 0;
    for (const auto& item : chromium) {
      permissions_[last_index++] = item;
    }
    for (const auto& item : brave) {
      permissions_[last_index++] = item;
    }
  }
  base::span<const T> GetPermissionInfos() const {
    return base::make_span(permissions_);
  }

 private:
  T permissions_[N];
};

constexpr size_t PermissionsTotal = base::size(permissions_to_register) +
                                    base::size(brave_permissions_to_register);

const PermissionsContainer<APIPermissionInfo::InitInfo, PermissionsTotal>
    final_permissions(base::make_span(permissions_to_register),
                      base::make_span(brave_permissions_to_register));

}  // namespace

base::span<const APIPermissionInfo::InitInfo> GetPermissionInfos() {
  return final_permissions.GetPermissionInfos();
}

}  // namespace chrome_api_permissions
}  // namespace extensions
