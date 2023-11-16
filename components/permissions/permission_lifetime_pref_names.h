/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_PREF_NAMES_H_
#define BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_PREF_NAMES_H_

namespace permissions {
namespace prefs {

// General pref for all permission lifetime logic.
inline constexpr char kPermissionLifetimeRoot[] = "permission_lifetime";
// Expiration pref to store currently expiring permissions.
inline constexpr char kPermissionLifetimeExpirations[] =
    "permission_lifetime.expirations";

}  // namespace prefs
}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_PREF_NAMES_H_
