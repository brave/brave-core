/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/brave_permission_manager.h"

#include <utility>

#include "components/permissions/permission_context_base.h"

namespace permissions {

BravePermissionManager::BravePermissionManager(
    content::BrowserContext* browser_context,
    PermissionContextMap permission_contexts)
    : PermissionManager(browser_context, std::move(permission_contexts)) {}

GURL BravePermissionManager::GetCanonicalOrigin(
    ContentSettingsType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) const {
  // Use requesting_origin which will have ethereum address info.
  if (permission == ContentSettingsType::BRAVE_ETHEREUM)
    return requesting_origin;

  return PermissionManager::GetCanonicalOrigin(permission, requesting_origin,
                                               embedding_origin);
}

}  // namespace permissions
